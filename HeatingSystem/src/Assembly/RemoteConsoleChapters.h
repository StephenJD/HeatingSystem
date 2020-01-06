#pragma once
#include "..\LCD_UI\A_Top_UI.h"
#include "..\LCD_UI\UI_FieldData.h"

namespace Assembly {
	class RemoteConsoleChapters : public LCD_UI::Chapter_Generator
	{
	public:
		RemoteConsoleChapters();
	private:
		// DB UIs (Lazy-Collections)
		// "^v adjusts temp"
		//LCD_UI::UI_FieldData _zoneIsReq_UI_c;

	};
}
