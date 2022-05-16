#include "windows.h"
#include <string.h>
#include <stdio.h>
#include "compat32.h"
#include "nxgo.h"
#include "xtgtrack.h"
#include "math.h"
#include <unordered_set>

#include <vector>   // Vectorized for global array and local segs 9/27/2019

#if TLEDIT
#include "undo.h"
#endif

#ifdef REALLY_NXSYS
#include "relays.h"
#include "trkload.h"
#include "xturnout.h"
#include "rlyapi.h"
#endif

/* Track circuits are not GraphicObjects and need explicit tracking.
   Track seg(ment)s, on the other hand, are GO's and are tracked by the
   GO system.
 */

static std::vector<TrackCircuit*> AllTrackCircuits;

TrackCircuit::TrackCircuit (long sno) {
    Occupied = Routed = Coding = FALSE;
    TrackRelay = NULL;
    StationNo = sno;
};

/* One can imagine that an std::set might be better if this is frequent,
 but it doesn't happen in real NXSYS, and in TLEdit is infrequent.*/
template <class T>
static void VEDEL (std::vector<T> v, T item) {  //1767-1808 Ukr. vector deleter
    for (auto it = v.begin(); it != v.end(); it++) {
        if (*it == item) {
            v.erase(it);
            break;
        }
    }
} 

TrackCircuit::~TrackCircuit () {
    VEDEL(AllTrackCircuits, this);
    for (auto ts : Segments)
	ts->Circuit = NULL;
}


static TrackCircuit* CreateNewTrackCircuit (long sno) {
    TrackCircuit* new_circuit = new TrackCircuit(sno);
    AllTrackCircuits.push_back(new_circuit);
    return new_circuit;
}

TrackCircuit* FindTrackCircuit (long sno) {
    for (TrackCircuit * tc : AllTrackCircuits)
	if (tc->StationNo == sno)
            return tc;
    return NULL;
}

TrackCircuit* GetTrackCircuit (long sno) {
    TrackCircuit * tc = FindTrackCircuit (sno);
    return tc ? tc : CreateNewTrackCircuit(sno);
}

void TrackCircuit::DeleteSeg (TrackSeg * ts) {
        VEDEL(Segments, ts);
}

void TrackCircuit::AddSeg (TrackSeg * ts) {
    ts->Circuit = this;
    for (auto ats : Segments)  // shouldn't really be in array
        if (ats == ts)
            return;
    Segments.push_back(ts);
}

#if TLEDIT
std::unordered_set<TrackSeg*> WildfireLog;
#endif

TrackCircuit * TrackSeg::SetTrackCircuit (long tcid, BOOL wildfire) {
    if (tcid == 0) {
        if (Circuit && !wildfire) {
            Circuit->DeleteSeg(this);
            Circuit = NULL;
            Invalidate();
        }
        return NULL;
    }
    TrackCircuit * tc = GetTrackCircuit (tcid);
    if (wildfire) {
#if TLEDIT
        long orig_tcid = Circuit ? Circuit->StationNo : 0;
        WildfireLog.clear();
        SetTrackCircuitWildfire (tc);
        Undo::RecordWildfireTCSpread(WildfireLog, (int)orig_tcid, (int)tcid);
        WildfireLog.clear();
#else
        SetTrackCircuitWildfire (tc);
#endif
    }
    else
	SetTrackCircuit0 (tc);
    return tc;
}

void TrackSeg::SetTrackCircuit0 (TrackCircuit * tc) {
    if (Circuit != tc) {
	if (Circuit)
	    Circuit->DeleteSeg(this);
	if (tc)
	    tc->AddSeg(this);
    }
}

void TrackSeg::SetTrackCircuitWildfire (TrackCircuit * tc) {
    if (Circuit == tc)
	return;
    SetTrackCircuit0 (tc);
#if TLEDIT
    WildfireLog.insert(this);
#endif
    
    for (int ex = 0; ex < 2; ex++) {
	TrackSegEnd* ep = &Ends[ex];
	if (ep->Joint && ep->Joint->Insulated)
	    continue;
#if TLEDIT
        if (TrackJoint * tj = ep->Joint) {
            for (int i = 0; i < tj->TSCount; i++) {
                if (tj->TSA[i] != this)
                    tj->TSA[i]->SetTrackCircuitWildfire(tc);
            }
        }
#else
	if (ep->Next)
	    ep->Next->SetTrackCircuitWildfire (tc);
	if (ep->NextIfSwitchThrown)
	    ep->NextIfSwitchThrown->SetTrackCircuitWildfire (tc);
#endif
    }
}

void TrackCircuit::SetOccupied (BOOL sta, BOOL force) {
    if ((Occupied != sta) || force) {
	Occupied = sta;
	Invalidate();
#ifdef REALLY_NXSYS
	if (TrackRelay)
	    ReportToRelay (TrackRelay, !Occupied);
#endif
    }
}

void TrackCircuit::SetRouted (BOOL sta) {
    if (Routed != sta) {
	Routed = sta;
	Invalidate();
    }
}

void TrackCircuit::Invalidate() {
    for (auto ts : Segments) {
#ifdef REALLY_NXSYS
	if (ts->Ends[0].Joint && ts->Ends[0].Joint->TurnOut)
	    ts->Ends[0].Joint->TurnOut->InvalidateAndTurnouts();
	if (ts->Ends[1].Joint && ts->Ends[1].Joint->TurnOut)
	    ts->Ends[1].Joint->TurnOut->InvalidateAndTurnouts();
#endif
	ts->Invalidate();
    }
}

#ifdef REALLY_NXSYS
void TrackCircuit::ProcessLoadComplete () {
    TrackRelay = CreateAndSetReporter (StationNo, "T", TrackReportFcn, this);
    CreateAndSetReporter (StationNo, "K", TrackKRptFcn, this);
}


void ReportAllTrackSecsClear () {
    for (auto tc : AllTrackCircuits)
	tc->SetOccupied(FALSE, TRUE); //force=
}


void ClearAllTrackSecs () {
    for (auto tc : AllTrackCircuits)
	tc->SetOccupied(FALSE);
}

void TrackCircuitSystemLoadTimeComplete () {
    for (auto tc : AllTrackCircuits)
	tc->ProcessLoadComplete();
}

void TrackCircuitSystemReInit() {
    std::unordered_set<TrackCircuit*> Circuits;
    for (auto tc : AllTrackCircuits)
        Circuits.insert(tc);
    for (auto ttc : Circuits)
        delete ttc;
    AllTrackCircuits.clear();
}

void TrackCircuit::TrackReportFcn(BOOL state, void* v) {
    TrackCircuit * t = (TrackCircuit *) v;
    t->Occupied = !state;
    t->Invalidate();
}

void TrackCircuit::TrackKRptFcn(BOOL state, void* v) {
    ((TrackCircuit *) v)->SetRouted (state);
}


void TrackCircuit::ComputeSwitchRoutedState () {
    /* route all segments that have both ends clear to an IJ with
       no trailed trailing point switches */

    for (auto ts : Segments)
	ts->ComputeSwitchRoutedState ();
}

void TrackSeg::ComputeSwitchRoutedState () {
    BOOL oldstate = Routed;
    Routed = ComputeSwitchRoutedEndState(0)
	     && ComputeSwitchRoutedEndState(1);
    if (oldstate != Routed)
	Invalidate();
}

BOOL TrackSeg::ComputeSwitchRoutedEndState(int ex) {
    TrackSegEnd * ep = &Ends[ex];
    if (ep->InsulatedP())
	return TRUE;
    if (ep->Next == NULL)
	return TRUE;
    TrackJoint * tj = ep->Joint;
    if (tj == NULL) {
next:
	return ep->Next->ComputeSwitchRoutedEndState(1-(int)ep->EndIndexNormal);
    }

    if (tj->TSCount < 3)
	return TRUE;
    if (tj->TurnOut == NULL)
	return TRUE;
    BOOL Thrown = tj->TurnOut->Thrown;
    if (this == (*tj)[TSAX::REVERSE])
	if (Thrown)
	    goto next;
	else
	    return FALSE;
    else if (this == (*tj)[TSAX::NORMAL])
	if (Thrown)
	    return FALSE;
	else
	    goto next;
    else {
	if (Thrown)
	    return ep->NextIfSwitchThrown
		    ->ComputeSwitchRoutedEndState(1-(int)ep->EndIndexReverse);
	else
	    goto next;
    }
}

GraphicObject * FindDemoHitCircuit (long id) {
    TrackCircuit * tc = FindTrackCircuit (id);
    if (tc == NULL)
	return NULL;
    return tc->FindDemoHitSeg();
}

TrackSeg * TrackCircuit::FindDemoHitSeg() {
    for (auto ts : Segments) {
	if (!ts->OwningTurnout)
	    return ts;
    }
    return Segments[0];
}

void TrackCircuit::ComputeOccupiedFromTrains () {
    int occ = 0;
    for (auto ts : Segments)
	occ += ts->TrainCount;
    SetOccupied ((BOOL)(occ != 0));
}


#endif


BOOL TrackCircuit::MultipleSegmentsP() {
    return Segments.size() > 1;
}
