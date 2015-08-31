// 16 may 2015
#include "uipriv_windows.h"

struct uiGroup {
	uiWindowsControl c;
	HWND hwnd;
	struct child *child;
	int margined;
};

static void onDestroy(uiGroup *);

uiWindowsDefineControlWithOnDestroy(
	uiGroup,								// type name
	uiGroupType,							// type function
	onDestroy(this);						// on destroy
)

static void onDestroy(uiGroup *g)
{
	if (g->child != NULL)
		childDestroy(g->child);
}

// from https://msdn.microsoft.com/en-us/library/windows/desktop/dn742486.aspx#sizingandspacing
#define groupXMargin 6
#define groupYMarginTop 11 /* note this value /includes the groupbox label */
#define groupYMarginBottom 7

// unfortunately because the client area of a groupbox includes the frame and caption text, we have to apply some margins ourselves, even if we don't want "any"
// these were deduced by hand based on the standard DLU conversions; the X and Y top margins are the width and height, respectively, of one character cell
// they can be fine-tuned later
#define groupUnmarginedXMargin 4
#define groupUnmarginedYMarginTop 8
#define groupUnmarginedYMarginBottom 3

static void minimumSize(uiControl *c, uiWindowsSizing *d, intmax_t *width, intmax_t *height)
{
	uiGroup *g = uiGroup(c);

	*width = 0;
	*height = 0;
	if (g->child != NULL)
		childMinimumSize(g->child, d, width, height);
	if (g->margined) {
		*width += 2 * uiWindowsDlgUnitsToX(groupXMargin, d->BaseX);
		*height += uiWindowsDlgUnitsToY(groupYMarginTop, d->BaseY) + uiWindowsDlgUnitsToY(groupYMarginBottom, d->BaseY);
	} else {
		*width += 2 * uiWindowsDlgUnitsToX(groupUnmarginedXMargin, d->BaseX);
		*height += uiWindowsDlgUnitsToY(groupUnmarginedYMarginTop, d->BaseY) + uiWindowsDlgUnitsToY(groupUnmarginedYMarginBottom, d->BaseY);
	}
}

static void groupRelayout(uiControl *c, intmax_t x, intmax_t y, intmax_t width, intmax_t height)
{
	struct group *g = (struct group *) c;
	uiSizing *d;

	// TODO
	(*(g->baseResize))(uiControl(g), x, y, width, height, d);

	if (g->child == NULL)
		return;

	d = uiWindowsGetSizing(g->hwnd);
	if (g->margined) {
		x += uiWindowsDlgUnitsToX(groupXMargin, d->BaseX);
		y += uiWindowsDlgUnitsToY(groupYMarginTop, d->BaseY);
		width -= 2 * uiWindowsDlgUnitsToX(groupXMargin, d->BaseX);
		height -= uiWindowsDlgUnitsToY(groupYMarginTop, d->BaseY) + uiWindowsDlgUnitsToY(groupYMarginBottom, d->BaseY);
	} else {
		x += uiWindowsDlgUnitsToX(groupUnmarginedXMargin, d->BaseX);
		y += uiWindowsDlgUnitsToY(groupUnmarginedYMarginTop, d->BaseY);
		width -= 2 * uiWindowsDlgUnitsToX(groupUnmarginedXMargin, d->BaseX);
		height -= uiWindowsDlgUnitsToY(groupUnmarginedYMarginTop, d->BaseY) + uiWindowsDlgUnitsToY(groupUnmarginedYMarginBottom, d->BaseY);
	}
	uiWindowsFreeSizing(d);
	childRelayout(g->child, x, y, width, height);
}

static void groupContainerUpdateState(uiControl *c)
{
	uiGroup *g = uiGroup(c);

	if (g->child != NULL)
		childUpdateState(g->child);
}

char *uiGroupTitle(uiGroup *g)
{
	return uiWindowsUtilText(g->hwnd);
}

void uiGroupSetTitle(uiGroup *g, const char *text)
{
	uiWindowsUtilSetText(g->hwnd, text);
	// changing the text might necessitate a change in the groupbox's size
	uiControlQueueResize(uiControl(g));
}

void uiGroupSetChild(uiGroup *g, uiControl *child)
{
	if (g->child != NULL)
		childRemove(g->child);
	g->child = newChild(child, uiControl(g), g->hwnd);
	if (g->child != NULL)
		uiControlQueueResize(g->child);
}

int uiGroupMargined(uiGroup *g)
{
	return g->margined;
}

void uiGroupSetMargined(uiGroup *g, int margined)
{
	g->margined = margined;
	uiControlQueueResize(uiControl(g));
}

uiGroup *uiNewGroup(const char *text)
{
	uiGroup *g;
	WCHAR *wtext;

	g = (uiGroup *) uiNewControl(uiGroupType());

	wtext = toUTF16(text);
	g->hwnd = uiWindowsEnsureCreateControlHWND(WS_EX_CONTROLPARENT,
		L"button", wtext,
		BS_GROUPBOX,
		hInstance, NULL,
		TRUE);
	uiFree(wtext);

	// TODO subclass uiGroup to call parent.c functions

	uiWindowsFinishNewControl(g, uiGroup);
	uiControl(g)->Resize = groupRelayout;
	uiControl(g)->ContainerUpdateState = groupContainerUpdateState;

	return g;
}
