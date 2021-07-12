#include "hudelement.h"
#include <vgui_controls/Panel.h>

using namespace vgui;
int cvvalue = 0;

class CHudKeycard_1 : public CHudElement, public Panel
{
	DECLARE_CLASS_SIMPLE(CHudKeycard_1, Panel);
public:
	CHudKeycard_1(const char *pElementName);
	void togglePrint();
	virtual void OnThink();
	void MapLoad_NewGame();

protected:
	virtual void Paint();
	int m_nImport;
};

class CHudKeycard_2 : public CHudElement, public Panel
{
	DECLARE_CLASS_SIMPLE(CHudKeycard_2, Panel);

public:
	CHudKeycard_2(const char *pElementName);
	void togglePrint();
	virtual void OnThink();
	void MapLoad_NewGame();

protected:
	virtual void Paint();
	int m_nImport;
};

class CHudKeycard_3 : public CHudElement, public Panel
{
	DECLARE_CLASS_SIMPLE(CHudKeycard_3, Panel);

public:
	CHudKeycard_3(const char *pElementName);
	void togglePrint();
	virtual void OnThink();
	void MapLoad_NewGame();

protected:
	virtual void Paint();
	int m_nImport;
};