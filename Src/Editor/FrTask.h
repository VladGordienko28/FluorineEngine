/*=============================================================================
    FrTask.h: Task processing dialog.
    Copyright Dec.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    WTaskDialog.
-----------------------------------------------------------------------------*/

//
// A dialog to show processing.
//
class WTaskDialog: public WForm
{
public:
	// WTaskDialog interface.
	WTaskDialog( WWindow* InRoot );
	~WTaskDialog();
	void Begin( String TaskName );
	void End();
	void UpdateSubtask( String SubtaskName );
	void UpdateProgress( Integer Numerator, Integer Denominator );

	// WForm interface.
	void Hide();

	// Accessors.
	inline Bool InProgress()
	{
		return bInProgress;
	}

private:
	// Internal.
	Bool			bInProgress;
	WForm*			OldModal;
	WLabel*			Label;
	WProgressBar*	ProgressBar;

	void RedrawAll();
};


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/