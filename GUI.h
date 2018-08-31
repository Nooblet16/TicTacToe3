#pragma once

/*
	Graphical user interface.
	
	Consists of a Win32 BITMAP where the text is drawn with Win32 API functions and  an OpenGL texture 
	where the bitmap's pixels are copied to present the GUI on screen with transparency.

	A collection of 'widgets' representing labels and buttons is maintained and rendered into the bitmap.
*/

struct GUI
{
	struct RGBA //Pixel structure of Win32 BITMAP
	{
		unsigned char B, G, R, A;
	};
	
	struct Widget //GUI widget: label, button or checkbox.
	{
		int pos[2];
		int size[2];
		const char* text;
		HFONT font;
		
		enum StateBits {
			Clickable = 1,
			Check = 2, //Some widgets have checked/unchecked state.
			Visible = 4,
		};
		int state; //A bit field storing widget's permanent and temporary properties.

		void init(int x, int y, int w, int h, const char* txt, HFONT fnt, int stat)
		{
			pos[0] = x;
			pos[1] = y;
			size[0] = w;
			size[1] = h;
			text = txt;
			font = fnt;
			state = stat;
		}
		bool hasPoint(int x, int y)
		{
			return (x>pos[0] && x<pos[0]+size[0] && y>pos[1] && y<pos[1]+size[1]);
		}
		void setBit(int bit, bool on)
		{
			if(on){
				state |= bit;
			}else{
				state &= ~bit;
			}
		}
	};
	
	static const int Reso = 512;//Native resolution for the bitmap and texture.
	HDC dc;
	HBITMAP bitmap;
	GLuint texture;
	RGBA* pixels;//A pointer to bitmap's pixels. Used to manually draw things into it.
	
	enum Screens {
		MainMenu = 1,
		Game,
		Result,
	};
	int screen;//Current GUI screen index: menu, game, session result.
	
	bool dirty;//This flag causes all widgets be redrawn on the next refresh.
	int widgetAtCursor;//The index of the widget the mouse cursor is hovering over.
	int opacity;//Whole menu opacity, [0..255], used to animate the 'fade in' effect.
	
	enum Widgets {
		//Menu screen widgets
		MenuBG = 0,
		Title,
		Title3,
		Player1,
		Player2,
		Human1,
		RandomAI1,
		HeuristicAI1,
		MinmaxAI1,
		Human2,
		RandomAI2,
		HeuristicAI2,
		MinmaxAI2,
		GridSize,
		GridSize3,
		GridSize4,
		GridSize5,
		GridSize6,
		ToWin,
		ToWin3,
		ToWin4,
		ToWin5,
		ToWin6,
		Play,
		
		//Game screen widgets
		Back,
		GoPlayer1,
		GoPlayer2,

		//Result screen widgets
		ResultBG,
		ResultCaption,
		ReturnMenu,
		PlayAgain,
		
		NWidgets
	};
	Widget widgets[NWidgets];
	GUI()
	{
		texture = 0;
		bitmap = 0;
		pixels = 0;
		screen = 0;
		dirty = true;
		widgetAtCursor = -1;
		opacity = 200;
		ZeroMemory(widgets, sizeof(Widget)*NWidgets);
	}
	void init()
	{
		dc = CreateCompatibleDC(0);
		SetBkMode(dc, TRANSPARENT);//This is needed so that text will have transparent background.
		createBitmap();
		createTexture();

		HFONT titleFont  = CreateFont(50, 0,0,0,0,0,0,0, DEFAULT_CHARSET, 0,0, ANTIALIASED_QUALITY, 0, "Arial Black");
		HFONT titleFont3 = CreateFont(30, 0,0,0,0,0,0,0, DEFAULT_CHARSET, 0,0, ANTIALIASED_QUALITY, 0, "Arial Black");
		HFONT font       = CreateFont(24, 0,0,0,0,0,0,0, DEFAULT_CHARSET, 0,0, ANTIALIASED_QUALITY, 0, "Tahoma");

		widgets[MenuBG   ].init( 80,  50,360,350, 0, 0, 0);
		widgets[Title    ].init( 80,  50,360, 60, "Tic Tac Toe", titleFont,  0);
		widgets[Title3   ].init(375,  60, 15, 25, "3",           titleFont3, 0);
		widgets[Player1  ].init( 90, 120, 90, 40, "Player 1",  font, 0);
		widgets[Player2  ].init( 90, 170, 90, 40, "Player 2",  font, 0);
		widgets[Human1   ].init(190, 120, 90, 40, "Human",     font, Widget::Clickable|Widget::Check);
		widgets[Human2   ].init(190, 170, 90, 40, "Human",     font, Widget::Clickable);
		widgets[RandomAI1].init(290, 120, 40, 40, "Rnd",  font, Widget::Clickable);
		widgets[RandomAI2].init(290, 170, 40, 40, "Rnd",  font, Widget::Clickable|Widget::Check);
		widgets[HeuristicAI1].init(340, 120, 40, 40, "Heu",  font, Widget::Clickable);
		widgets[HeuristicAI2].init(340, 170, 40, 40, "Heu",  font, Widget::Clickable);
		widgets[MinmaxAI1].init(390, 120, 40, 40, "MM",  font, Widget::Clickable);
		widgets[MinmaxAI2].init(390, 170, 40, 40, "MM",  font, Widget::Clickable);
		
		widgets[GridSize ].init( 90, 220, 90, 40, "Grid size", font, 0);
		widgets[GridSize3].init(190, 220, 40, 40, "3",         font, Widget::Clickable|Widget::Check);
		widgets[GridSize4].init(240, 220, 40, 40, "4",         font, Widget::Clickable);
		widgets[GridSize5].init(290, 220, 40, 40, "5",         font, Widget::Clickable);
		widgets[GridSize6].init(340, 220, 40, 40, "6",         font, Widget::Clickable);
		widgets[ToWin    ].init( 90, 270, 90, 40, "To win",    font, 0);
		widgets[ToWin3   ].init(190, 270, 40, 40, "3",         font, Widget::Clickable|Widget::Check);
		widgets[ToWin4   ].init(240, 270, 40, 40, "4",         font, 0);
		widgets[ToWin5   ].init(290, 270, 40, 40, "5",         font, 0);
		widgets[ToWin6   ].init(340, 270, 40, 40, "6",         font, 0);
		widgets[Play     ].init(190, 330,100, 50, "Play",      font, Widget::Clickable);

		widgets[Back     ].init(0,   10, 80,50, "Quit",       font, Widget::Clickable);
		widgets[GoPlayer1].init(100, 10,100,40, "> Player 1", font, 0);
		widgets[GoPlayer2].init(300, 10,100,40, "> Player 2", font, 0);

		widgets[ResultBG     ].init( 50, 10,400,100, 0, 0, 0);
		widgets[ResultCaption].init( 50, 10,400, 50, "Player X wins!", font, 0);
		widgets[ReturnMenu   ].init( 80, 60,180, 40, "Return to menu", font, Widget::Clickable);
		widgets[PlayAgain    ].init(300, 60,120, 40, "Play again",     font, Widget::Clickable);
	}
	void createTexture()
	{
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}
	void createBitmap()
	{
		BITMAPINFO bi;
		ZeroMemory(&bi, sizeof(BITMAPINFO));
		bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bi.bmiHeader.biBitCount = 32;
		bi.bmiHeader.biWidth = 
		bi.bmiHeader.biHeight = Reso;
		bi.bmiHeader.biPlanes = 1;
		bi.bmiHeader.biCompression = BI_RGB;
		bitmap = CreateDIBSection(0, &bi, DIB_RGB_COLORS, (void**)&pixels, NULL, 0);
		SelectBitmap(dc, bitmap);
	}
	void clear()
	{
		ZeroMemory(pixels, sizeof(int)*Reso*Reso);
	}
	void draw()
	{
		if(dirty){
			repaint();
			dirty = false;
		}
		glBindTexture(GL_TEXTURE_2D, texture);
		glBegin(GL_TRIANGLE_FAN);
			glTexCoord2f(0,0); glVertex2f(-1,-1);
			glTexCoord2f(1,0); glVertex2f( 1,-1);
			glTexCoord2f(1,1); glVertex2f( 1, 1);
			glTexCoord2f(0,1); glVertex2f(-1, 1);
		glEnd();

		if(opacity < 200){//Animate the 'fade in' effect. GUI transitions from transparent to opaque across multiple frames.
			opacity += 20;
			dirty = true;
		}
	}
	void repaint()
	{
		clear();
		for(int i=0; i<NWidgets; i++){
			Widget& w = widgets[i];
			if(! (w.state & Widget::Visible)){
				continue;
			}
			for(int y=0; y<w.size[1]; y++){
				for(int x=0; x<w.size[0]; x++){
					int X = x + w.pos[0];
					int Y = Reso-1 - (y + w.pos[1]);
					if(w.state & Widget::Check){
						if(i>=Human1 && i<=MinmaxAI1){
							//Paint Human/Computer switch buttons for Player1 with Player1's color.
							pixels[X+Y*Reso].R = 250;
							pixels[X+Y*Reso].G = 50;
							pixels[X+Y*Reso].B = 50;
						}else if(i>=Human2 && i<=MinmaxAI2){
							//Paint Human/Computer switch buttons fro Player2 with Player2's color.
							pixels[X+Y*Reso].R = 100;
							pixels[X+Y*Reso].G = 100;
							pixels[X+Y*Reso].B = 250;
						}else{
							//Paint generic switch button.
							pixels[X+Y*Reso].R = 50;
							pixels[X+Y*Reso].G = 200;
							pixels[X+Y*Reso].B = 50;
						}
					}else if(w.state & Widget::Clickable){
						if(i == widgetAtCursor){
							//Slightly highight button under the mouse cursor.
							pixels[X+Y*Reso].R = 200;
							pixels[X+Y*Reso].G = 200;
							pixels[X+Y*Reso].B = 200;
						}else{
							//Default button color.
							pixels[X+Y*Reso].R = 170;
							pixels[X+Y*Reso].G = 170;
							pixels[X+Y*Reso].B = 170;
						}
					}else{
						//Default widget color (label and backdrop widgets).
						pixels[X+Y*Reso].R = 250;
						pixels[X+Y*Reso].G = 250;
						pixels[X+Y*Reso].B = 250;
					}
				}
			}
			if(w.text){
				SelectFont(dc, w.font);
				drawTextXY(w.pos[0]+w.size[0]/2, w.pos[1]+w.size[1]/2, w.text);
			}
			for(int y=0; y<w.size[1]; y++){
				for(int x=0; x<w.size[0]; x++){
					int X = x + w.pos[0];
					int Y = Reso-1 - (y + w.pos[1]);
					pixels[X+Y*Reso].A = opacity;
				}
			}
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Reso, Reso, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels);
	}
	void drawTextXY(int x, int y, const char* text)
	{
		RECT size = {0, 0, 0, 0};
		DrawText(dc, text, -1, &size, DT_CALCRECT);
		int flags = DT_CENTER | DT_VCENTER | DT_NOCLIP;
		RECT rect = {x-size.right/2, y-size.bottom/2, x+size.right/2, y+size.bottom/2};
		DrawText(dc, text, -1, &rect, flags);
	}
	void onMouseDown(int x, int y, RECT& clientRect)
	{
		int w = widgetAtCursor;
		if(w>=Human1 && w<=MinmaxAI1){
			for(int i=Human1; i<=MinmaxAI1; i++){
				setWidgetCheck(i, w==i);
			}
		}
		if(w>=Human2 && w<=MinmaxAI2){
			for(int i=Human2; i<=MinmaxAI2; i++){
				setWidgetCheck(i, w==i);
			}
		}
		if(w>=GridSize3 && w<=GridSize6){
			int gridSize = w - GridSize3;
			for(int i=GridSize3; i<=GridSize6; i++){
				setWidgetCheck(i, w==i);
			}
			//Disable "ToWin" widgets correspondind to numbers higher than grid resolution.
			for(int i=ToWin3; i<=ToWin6; i++){
				int g = i - ToWin3;
				setWidgetClickable(i, g<=gridSize);
				if(g>gridSize && widgets[i].state & Widget::Check){
					setWidgetCheck(i, false);
					setWidgetCheck(ToWin3+gridSize, true);
				}
			}
		}
		if(w>=ToWin3 && w<=ToWin6){
			for(int i=ToWin3; i<=ToWin6; i++){
				setWidgetCheck(i, w==i);
			}
		}
		if(w == Play){
			setScreen(Game);
		}
		if(w == Back){
			setScreen(MainMenu);
		}
		if(w == ReturnMenu){
			setScreen(MainMenu);
		}
		if(widgetAtCursor == PlayAgain){
			setScreen(Game);
		}
	}
	void onMouseMove(int x, int y, RECT& clientRect)
	{
		transform(x, y, clientRect);
		//Determine widget under the cursor.
		int wat = getWidgetAtPoint(x, y);
		if(wat != widgetAtCursor){
			widgetAtCursor = wat;
			dirty = true;
		}
	}
	int getWidgetAtPoint(int x, int y)
	{
		for(int i=0; i<NWidgets; i++){
			if(widgets[i].state & Widget::Visible){
				if(widgets[i].state & Widget::Clickable){
					if(widgets[i].hasPoint(x, y)){
						return i;
					}
				}
			}
		}
		return -1;
	}
	void transform(int& x, int& y, RECT& clientRect)
	{
		//Map point from the window into the bitmap pixel coordinates.
		y = y * Reso / clientRect.bottom;
		x = (x-(clientRect.right-clientRect.bottom)/2) * Reso / clientRect.bottom;
	}
	//Change current screen.
	//This involves showing the widgets from that screen an hiding all the rest.
	void setScreen(int scr)
	{
		if(scr != screen){
			for(int w=0; w<NWidgets; w++){
				if(w < Back){
					widgets[w].setBit(Widget::Visible, scr==MainMenu);
				}else if(w < ResultBG){
					widgets[w].setBit(Widget::Visible, scr==Game);
				}else{
					widgets[w].setBit(Widget::Visible, scr==Result);
				}
			}
			//These two widgets are invisible by default, toggled by the game logic.
			widgets[GoPlayer1].setBit(Widget::Visible, false);
			widgets[GoPlayer2].setBit(Widget::Visible, false);
			screen = scr;
			opacity = 0;//Start GUI "fade in" animation.
			dirty = true;
		}
	}
	void setWidgetClickable(int w, bool value)
	{
		if(!!(widgets[w].state & Widget::Clickable) != value){
			widgets[w].setBit(Widget::Clickable, value);
			dirty = true;
		}
	}
	void setWidgetCheck(int w, bool value)
	{
		if(!!(widgets[w].state & Widget::Check) != value){
			widgets[w].setBit(Widget::Check, value);
			dirty = true;
		}
	}
	void setWidgetVisible(int w, bool value)
	{
		if(!!(widgets[w].state & Widget::Visible) != value){
			widgets[w].setBit(Widget::Visible, value);
			dirty = true;
		}
	}
	void setWidgetText(int w, const char* text)
	{
		widgets[w].text = text;
		dirty = true;
	}
	void setOpacity(int opa)
	{
		if(opa != opacity){
			opacity = opa;
			dirty = true;
		}
	}
	int getGridSize()
	{
		for(int i=GridSize3; i<=GridSize6; i++){
			if(widgets[i].state & Widget::Check){
				return 3+(i-GridSize3);
			}
		}
		return 3;
	}
	int getToWin()
	{
		for(int i=ToWin3; i<=ToWin6; i++){
			if(widgets[i].state & Widget::Check){
				return 3+(i-ToWin3);
			}
		}
		return 3;
	}
	int getPlayerType(int player)
	{
		if(player == 1){
			for(int i=Human1; i<=MinmaxAI1; i++){
				if(widgets[i].state & Widget::Check){
					return i-Human1;
				}
			}
		}
		if(player == 2){
			for(int i=Human2; i<=MinmaxAI2; i++){
				if(widgets[i].state & Widget::Check){
					return i-Human2;
				}
			}
		}
		return false;
	}
};
