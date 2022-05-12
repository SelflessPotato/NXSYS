#ifndef _NXSYS_EXTENDED_TRACK_GEOMETRY_H__
#define _NXSYS_EXTENDED_TRACK_GEOMETRY_H__

/* NXSYS Extended Geometry System  1 January 1997 */

#include <vector>   // 27 Sept 2019 for SegArray
#include "nxgo.h"
#ifndef TLEDIT
#ifndef REALLY_NXSYS
#define REALLY_NXSYS
#endif
#endif

class Relay;
class Signal;
class ExitLight;
class TrackSeg;
class Stop;

#ifdef REALLY_NXSYS
class Turnout;
#endif
#ifdef TLEDIT
#include "TLDlgProc.h"
#endif

/* Track section (branch) array index(es)*/
enum class TSAX {

    /* if TSCount = 3 (switch) */
    STEM   = 0,
    NORMAL = 1,
    REVERSE = 2,


    /* if TSCount = 2 (nonterminal IJ/kink) */
    IJR0 = 0,
    IJR1 = 1,
    IJRMAX = 1,

    NOTFOUND = -1,
    MIN = 0,
    MAX = 2
};

/* Track Section end index(es) */
enum class TSEX {

    E0 = 0,   /* meaningful items should appear first (for debugger) */
    E1 = 1,

    NOTFOUND = -1,
    MIN = 0,
    MAX = 1
};

struct JointOrganizationData {
    double Radang;
    double RRadang;
    double Interang;
    int original_index;
    BOOL SignalExists;
    TrackSeg * TSeg, *OpposingSeg;
};


class TrackJoint
#ifdef TLEDIT
   : public GraphicObject
#endif
{
    public:
	TrackJoint (WP_cord wpx1, WP_cord wpy1);
	~TrackJoint();

    TrackSeg* operator [](TSAX branch_index) {
        return GetBranch(branch_index);
    };

	long	Nomenclature;
	BOOL	Insulated;
	BOOL	NumFlip;
#ifdef TLEDIT
	BOOL    Marked;
	BOOL	Organized;
        bool    EditAsJointInProgress;
        bool    EditAsSwitchP() {
            return (TSCount == 3) && !EditAsJointInProgress;
        }
#else
	WP_cord wp_x, wp_y;
	Turnout * TurnOut;
#endif
	short   SwitchAB0;
	TrackSeg *TSA[3];
	int TSCount;
	NXGOLabel *Lab;

	int	AvailablePorts();
	BOOL	AddBranch(TrackSeg * ts);
	void	DelBranch(TrackSeg * ts);
	void	Organize();
	void    GetOrganization(JointOrganizationData*);
	void	PositionLabel();
	TSAX	FindBranchIndex (TrackSeg * ts);
        TrackSeg* GetBranch(TSAX brx);
    
	virtual void Display (HDC dc);
	virtual TypeId TypeID ();
	virtual bool IsNomenclature(long);
#ifdef TLEDIT

	virtual void Select();
	virtual void ShiftLayout2();
	virtual void Cut();
	virtual BOOL_DLG_PROC_QUAL DlgProc (HWND hDlg, UINT msg, WPARAM, LPARAM);
	virtual UINT DlgId();
	virtual BOOL ClickToSelectP();
	void	MoveToNewWPpos (WP_cord wpx1, WP_cord wpy1);
	void	SwallowOtherJoint (TrackJoint * tj);
        void	TDump (FILE * F, TSAX branch);
	void	EnsureID();
	void	Insulate();
	void	FlipNum();
	int	SignalCount();
	BOOL_DLG_PROC_QUAL SwitchDlgProc (HWND hDlg, UINT msg, WPARAM, LPARAM);
#endif

#ifdef REALLY_NXSYS
	int	StationNumber();	/* mod off Nomenclature */
        void    Consume() {};
#endif
};

class TrackCircuit {		// no longer graphic object
    public:
	BOOL  Occupied, Routed, Coding;
	Relay* TrackRelay;
        std::vector<TrackSeg*>Segments;
	long StationNo;
	void AddSeg (TrackSeg*);
	void DeleteSeg (TrackSeg*);
	void SetOccupied (BOOL sta, BOOL force=0);
	void SetRouted (BOOL sta);
	void Invalidate();
	void ProcessLoadComplete();

	TrackCircuit(long sno);
	~TrackCircuit();
	
#ifdef REALLY_NXSYS
	void ComputeOccupiedFromTrains();
	void ComputeSwitchRoutedState ();
	TrackSeg * FindDemoHitSeg();
	static void TrackReportFcn(BOOL state, void* v);
	static void TrackKRptFcn(BOOL state, void* v);

#endif

	BOOL MultipleSegmentsP();
};


class TrackSegEnd {			//NOT a graphic object
    public:
	RW_cord rwx, rwy;      //meaning of y not so clear
	WP_cord wpx, wpy;      //Windows (all-scrolled-out) Panel coord
#ifdef REALLY_NXSYS
	TrackSeg *Next;   //if there is a switch, "normal" next
	TrackSeg *NextIfSwitchThrown; // "reverse" next, if switch...
	Turnout *FacingSwitch; 
#endif
	Signal * SignalProtectingEntrance; //Train sys uses this for instruc
	TSEX EndIndexNormal, EndIndexReverse;
	ExitLight * ExLight;   // Has to know to redisplay this when lit.
	TrackJoint*Joint;
#ifdef REALLY_NXSYS
	void OffExitLight();
	void SpreadRWFactor (double rwf);
#endif	
	TrackSegEnd();
	BOOL InsulatedP();
	void Reposition();
};


class TrackSeg : public GraphicObject {
    public:
	TrackSegEnd Ends[2];
	TrackCircuit * Circuit;
	float   Length ;  // pythagoric, useful for drawing.
	float   CosTheta, SinTheta;
	BOOL    Routed;// this means "not the deselectd end of a trailing pt"
                      //sw, i.e., to be lit red/white in curr. sw. pos.
#ifdef TLEDIT
	BOOL    Marked;
#else
	Turnout *OwningTurnout;
	float   RWLength;  /* real-world length for train sys */
#endif
	short TrainCount;
	short GraphicBlips;
	TrackSeg (WP_cord wpx1, WP_cord wpy1, WP_cord wpx2, WP_cord wpy2);
	void DisplayInState (HDC dc, int state);
	BOOL SnapIntoLine (WP_cord& wpx, WP_cord& wpy);
	TrackJoint * FindOtherJoint (TrackJoint * tj);
	void Align(WP_cord wpx1, WP_cord wpy1, WP_cord wpx2, WP_cord wpy2);
	void Align();
	void Split(WP_cord wpx1, WP_cord wpy1, TrackJoint* tj);
	~TrackSeg();

	virtual void Display (HDC dc);
	virtual TypeId TypeID ();
	virtual bool IsNomenclature(long);
	TrackCircuit* SetTrackCircuit (long ID, BOOL wildfire);
	void SetTrackCircuit0 (TrackCircuit * tc);
	void SetTrackCircuitWildfire (TrackCircuit * tc);
	void GetGraphicsCoords (int ex, int& x, int& y);
	virtual BOOL HitP (long x, long y);
	BOOL HasCircuitBrothers();
        TSEX FindEndIndex (TrackJoint * tj);
        TrackSegEnd& GetEnd(TSEX ex);
        TrackSegEnd& GetOtherEnd(TSEX ex);
#ifdef TLEDIT
	char         EndOrientationKey (TSEX whichend);
	virtual void Select();
	virtual void Cut();
	void         SelectMsg();
	virtual BOOL_DLG_PROC_QUAL DlgProc (HWND hDlg, UINT msg, WPARAM, LPARAM);
	virtual UINT DlgId();
	virtual BOOL ClickToSelectP();
#else
	void SpreadSwitchRoutingState(BOOL p_routed);
	void SpreadRWFactor (double rwf);
	void ProcessLoadComplete();
	void ComputeSwitchRoutedState();
	BOOL ComputeSwitchRoutedEndState(int ex);
	int  StationPointsEnd (WP_cord &wpcordlen, TSEX end_index, int loop_check);
	virtual void EditContextMenu(HMENU m);

/*  Trains */
private:
        void UpdateCircuitOccupation();
public:
        void IncrementTrainOccupation();
        void DecrementTrainOccupation();

	virtual void Hit (int mb);
#endif

};


class PanelSignal  : public GraphicObject {
    public:    
	PanelSignal(TrackSeg * ts, TSEX endx, Signal * s, char * text);
	~PanelSignal();

	Signal * Sig;			/* operational logic object */

	TrackSeg * Seg;
	TSEX       EndIndex;

	WP_cord TRelX, TRelY;		/* track-aligned rel to IJ */
	WP_cord Radius;			/* head radius */
	NXGOLabel * Label;		/* labelling info */

	void GetAngles (float &, float &);
	void GetDispPoints (WP_cord &x1, WP_cord &y1,
			    WP_cord &x2, WP_cord &y2,
			    WP_cord &x3, WP_cord &y3);
	void LimWPCoord (WP_cord x, WP_cord y);
	void SetXlkgNo (int xlkgno, BOOL compvisible);
	void PositionLabel (BOOL compvisible);
        void ProcessLoadComplete();

#ifdef TLEDIT
	char Orientation();

	virtual void Cut();
	virtual BOOL_DLG_PROC_QUAL DlgProc (HWND hDlg, UINT msg, WPARAM, LPARAM);
    BOOL DlgOK(HWND hDlg);
	virtual UINT DlgId();
#else
	virtual void EditContextMenu(HMENU m);
	void DrawFleeting(HDC dc, BOOL enabled);
#endif
	virtual void Display (HDC dc);
	void Reposition();
	virtual TypeId TypeID ();
	virtual bool IsNomenclature(long);
	virtual void ComputeWPRect();
#ifdef TLEDIT
	virtual void Select();
	virtual int Dump (ObjectWriter& W);
#else
	virtual void Hit (int mb);
	virtual void UnHit();
#endif

};


class Stop : public GraphicObject{
public:
    Signal     *Sig;
    char	Tripping;
    Relay      *NVP, *RVP, *VPB;
    Stop(Signal * g);

    WP_cord x1, y1, x2, y2, x3, y3;

    int        coding;	       
    int	       code_clicks;

    void       Reposition();
    int        PressStopPB();
    void       StopCoder(BOOL state);
    void       VReporter(BOOL state);
    void       CodingDisplay();
    void       SigWinDisplay (HDC hDC, RECT * rect);
    void       ProcessLoadComplete();


    static  void  VReporterReporter(BOOL state, void* v);
    static  void  StopCoderReporter(void *v, BOOL state);

    virtual void Display (HDC dc) override;
    virtual TypeId TypeID() override;
    virtual bool IsNomenclature(long) override;
    virtual BOOL HitP(long, long) override;
#if REALLY_NXSYS   // if really nxsys, override with "false"; default is true.
    virtual bool MouseSensitive() override;
#endif
};

#pragma pack (push,xl)
#pragma pack(1)

/* Same name as V1 exit light, but  quite different. Of course,
   some methods in common. */
#define EXLIGHT_DEFINED 1

class ExitLight : public GraphicObject {
public:
    TrackSeg * Seg;
    int  XlkgNo;
    Relay * XPB;

    TSEX EndIndex;
    unsigned char Lit;
    unsigned char RedFlash;
    unsigned char Blacking;


public:

    ExitLight (TrackSeg * seg, TSEX EndIndex, int xno);
    ~ExitLight();
    void DisplayExit (HDC hdc, int sw);
    void Reposition();
    void SetLit (BOOL onoff);

    virtual void Display (HDC hdc);
    virtual BOOL HitP (long x, long y);
    virtual TypeId TypeID();
    virtual bool IsNomenclature(long);
#ifdef TLEDIT
    virtual int Dump (ObjectWriter& W);
    virtual void Select();
    virtual void Cut();
    virtual BOOL_DLG_PROC_QUAL DlgProc (HWND hDlg, UINT msg, WPARAM, LPARAM);
    virtual UINT DlgId();
#else 
    virtual void Hit (int mb);
    void SetRedFlash (BOOL onoff);
    void ProcessLoadComplete();
    void DrawBlack();
    void Off();
#endif
    static void KReporter(BOOL state, void*);
};

#pragma pack (pop,xl)

TrackSeg * SnapToTrackSeg (WP_cord& wpx, WP_cord& wpy);
extern BOOL ShowNonselectedJoints;

#ifdef REALLY_NXSYS
void TrackCircuitSystemLoadTimeComplete();
void TrackCircuitSystemReInit();
void DecodeDigitated (long input, int &trackno, int &sno);
TrackCircuit * FindTrackCircuit (long sno);
void TrackCircuitSystemReInit();
#endif

#endif
