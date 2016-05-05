#include "EmulationStation.h"
#include "guis/GuiMenu.h"
#include "Window.h"
#include "Sound.h"
#include "Log.h"
#include "Settings.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiSettings.h"
#include "guis/GuiScraperStart.h"
#include "guis/GuiDetectDevice.h"
#include "views/ViewController.h"

#include "components/ButtonComponent.h"
#include "components/SwitchComponent.h"
#include "components/SliderComponent.h"
#include "components/TextComponent.h"
#include "components/OptionListComponent.h"
#include "components/MenuComponent.h"
#include "VolumeControl.h"
#include "scrapers/GamesDBScraper.h"
#include "scrapers/TheArchiveScraper.h"

GuiMenu::GuiMenu(Window* window) : GuiComponent(window), mMenu(window, "MENU PRINCIPAL"), mVersion(window)
{
	// MAIN MENU

	// SCRAPER >
	// SOUND SETTINGS >
	// UI SETTINGS >
	// CONFIGURE INPUT >
	// QUIT >

	// [version]

	auto openScrapeNow = [this] { mWindow->pushGui(new GuiScraperStart(mWindow)); };
	addEntry("SCRAPER", 0x777777FF, true, 
		[this, openScrapeNow] { 
			auto s = new GuiSettings(mWindow, "SCRAPER");

			// scrape from
			auto scraper_list = std::make_shared< OptionListComponent< std::string > >(mWindow, "BUSCAR DE", false);
			std::vector<std::string> scrapers = getScraperList();
			for(auto it = scrapers.begin(); it != scrapers.end(); it++)
				scraper_list->add(*it, *it, *it == Settings::getInstance()->getString("Scraper"));

			s->addWithLabel("BUSCAR DE", scraper_list);
			s->addSaveFunc([scraper_list] { Settings::getInstance()->setString("Scraper", scraper_list->getSelected()); });

			// scrape ratings
			auto scrape_ratings = std::make_shared<SwitchComponent>(mWindow);
			scrape_ratings->setState(Settings::getInstance()->getBool("ScrapeRatings"));
			s->addWithLabel("BUSCAR CLASSIFICACOES", scrape_ratings);
			s->addSaveFunc([scrape_ratings] { Settings::getInstance()->setBool("ScrapeRatings", scrape_ratings->getState()); });

			// scrape now
			ComponentListRow row;
			std::function<void()> openAndSave = openScrapeNow;
			openAndSave = [s, openAndSave] { s->save(); openAndSave(); };
			row.makeAcceptInputHandler(openAndSave);

			auto scrape_now = std::make_shared<TextComponent>(mWindow, "INICIAR BUSCA", Font::get(FONT_SIZE_MEDIUM), 0x777777FF);
			auto bracket = makeArrow(mWindow);
			row.addElement(scrape_now, true);
			row.addElement(bracket, false);
			s->addRow(row);

			mWindow->pushGui(s);
	});

	addEntry("DEFINICOES DE SOM", 0x777777FF, true, 
		[this] {
			auto s = new GuiSettings(mWindow, "DEFINICOES DE SOM");

			// volume
			auto volume = std::make_shared<SliderComponent>(mWindow, 0.f, 100.f, 1.f, "%");
			volume->setValue((float)VolumeControl::getInstance()->getVolume());
			s->addWithLabel("VOLUME DO SISTEMA", volume);
			s->addSaveFunc([volume] { VolumeControl::getInstance()->setVolume((int)round(volume->getValue())); });
			
			// disable sounds
			auto sounds_enabled = std::make_shared<SwitchComponent>(mWindow);
			sounds_enabled->setState(Settings::getInstance()->getBool("EnableSounds"));
			s->addWithLabel("ATIVAR SONS", sounds_enabled);
			s->addSaveFunc([sounds_enabled] { Settings::getInstance()->setBool("EnableSounds", sounds_enabled->getState()); });

			mWindow->pushGui(s);
	});

	addEntry("DEFINICOES DE TELA", 0x777777FF, true,
		[this] {
			auto s = new GuiSettings(mWindow, "DEFINICOES DE TELA");

			// screensaver time
			auto screensaver_time = std::make_shared<SliderComponent>(mWindow, 0.f, 30.f, 1.f, "m");
			screensaver_time->setValue((float)(Settings::getInstance()->getInt("ScreenSaverTime") / (1000 * 60)));
			s->addWithLabel("PROTECAO DE TELA APOS", screensaver_time);
			s->addSaveFunc([screensaver_time] { Settings::getInstance()->setInt("ScreenSaverTime", (int)round(screensaver_time->getValue()) * (1000 * 60)); });

			// screensaver behavior
			auto screensaver_behavior = std::make_shared< OptionListComponent<std::string> >(mWindow, "ESTILO DE TRANSICAO", false);
			std::vector<std::string> screensavers;
			screensavers.push_back("dim");
			screensavers.push_back("black");
			for(auto it = screensavers.begin(); it != screensavers.end(); it++)
				screensaver_behavior->add(*it, *it, Settings::getInstance()->getString("ScreenSaverBehavior") == *it);
			s->addWithLabel("COMPORTAMENTO DO PROTETOR DE TELA", screensaver_behavior);
			s->addSaveFunc([screensaver_behavior] { Settings::getInstance()->setString("ScreenSaverBehavior", screensaver_behavior->getSelected()); });

			// framerate
			//auto framerate = std::make_shared<SwitchComponent>(mWindow);
			//framerate->setState(Settings::getInstance()->getBool("DrawFramerate"));
			//s->addWithLabel("SHOW FRAMERATE", framerate);
			//s->addSaveFunc([framerate] { Settings::getInstance()->setBool("DrawFramerate", framerate->getState()); });

			// show help
			//auto show_help = std::make_shared<SwitchComponent>(mWindow);
			//show_help->setState(Settings::getInstance()->getBool("ShowHelpPrompts"));
			//s->addWithLabel("ON-SCREEN HELP", show_help);
			//s->addSaveFunc([show_help] { Settings::getInstance()->setBool("ShowHelpPrompts", show_help->getState()); });

			// quick system select (left/right in game list view)
			auto quick_sys_select = std::make_shared<SwitchComponent>(mWindow);
			quick_sys_select->setState(Settings::getInstance()->getBool("QuickSystemSelect"));
			s->addWithLabel("SELECAO RAPIDA DE CONSOLE", quick_sys_select);
			s->addSaveFunc([quick_sys_select] { Settings::getInstance()->setBool("QuickSystemSelect", quick_sys_select->getState()); });

			// transition style
			auto transition_style = std::make_shared< OptionListComponent<std::string> >(mWindow, "ESTILO DE TRANSICAO", false);
			std::vector<std::string> transitions;
			transitions.push_back("fade");
			transitions.push_back("slide");
			for(auto it = transitions.begin(); it != transitions.end(); it++)
				transition_style->add(*it, *it, Settings::getInstance()->getString("TransitionStyle") == *it);
			s->addWithLabel("ESTILO DE TRANSICAO", transition_style);
			s->addSaveFunc([transition_style] { Settings::getInstance()->setString("TransitionStyle", transition_style->getSelected()); });

			// theme set
			auto themeSets = ThemeData::getThemeSets();

			if(!themeSets.empty())
			{
				auto selectedSet = themeSets.find(Settings::getInstance()->getString("ThemeSet"));
				if(selectedSet == themeSets.end())
					selectedSet = themeSets.begin();

				auto theme_set = std::make_shared< OptionListComponent<std::string> >(mWindow, "TEMA", false);
				for(auto it = themeSets.begin(); it != themeSets.end(); it++)
					theme_set->add(it->first, it->first, it == selectedSet);
				s->addWithLabel("TEMA", theme_set);

				Window* window = mWindow;
				s->addSaveFunc([window, theme_set] 
				{
					bool needReload = false;
					if(Settings::getInstance()->getString("ThemeSet") != theme_set->getSelected())
						needReload = true;

					Settings::getInstance()->setString("ThemeSet", theme_set->getSelected());

					if(needReload)
						ViewController::get()->reloadAll(); // TODO - replace this with some sort of signal-based implementation
				});
			}

			mWindow->pushGui(s);
	});

	//addEntry("OTHER SETTINGS", 0x777777FF, true,
	//	[this] {
	//		auto s = new GuiSettings(mWindow, "OTHER SETTINGS");
//
//			// gamelists
//			auto save_gamelists = std::make_shared<SwitchComponent>(mWindow);
//			save_gamelists->setState(Settings::getInstance()->getBool("SaveGamelistsOnExit"));
//			s->addWithLabel("SAVE METADATA ON EXIT", save_gamelists);
//			s->addSaveFunc([save_gamelists] { Settings::getInstance()->setBool("SaveGamelistsOnExit", save_gamelists->getState()); });
//
//			auto parse_gamelists = std::make_shared<SwitchComponent>(mWindow);
//			parse_gamelists->setState(Settings::getInstance()->getBool("ParseGamelistOnly"));
//			s->addWithLabel("PARSE GAMESLISTS ONLY", parse_gamelists);
//			s->addSaveFunc([parse_gamelists] { Settings::getInstance()->setBool("ParseGamelistOnly", parse_gamelists->getState()); });
//
//			mWindow->pushGui(s);
//	});

	addEntry("CONFIGURAR CONTROLE", 0x777777FF, true, 
		[this] { 
			mWindow->pushGui(new GuiDetectDevice(mWindow, false, nullptr));
	});

	addEntry("SAIR", 0x777777FF, true, 
		[this] {
			auto s = new GuiSettings(mWindow, "SAIR");

			Window* window = mWindow;

			ComponentListRow row;
		//	row.makeAcceptInputHandler([window] {
		//		window->pushGui(new GuiMsgBox(window, "DESEJA REINICIAR?", "SIM",
		//		[] {
		//			if(quitES("/tmp/es-restart") != 0)
		//				LOG(LogWarning) << "Restart terminated with non-zero result!";
		//		}, "NO", nullptr));
		//	});
		//	row.addElement(std::make_shared<TextComponent>(window, "RESTART EMULATIONSTATION", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
		//	s->addRow(row);

			row.elements.clear();
			row.makeAcceptInputHandler([window] {
				window->pushGui(new GuiMsgBox(window, "DESEJA REINICIAR?", "SIM", 
				[] { 
					if(quitES("/tmp/es-sysrestart") != 0)
						LOG(LogWarning) << "Restart terminated with non-zero result!";
				}, "NAO", nullptr));
			});
			row.addElement(std::make_shared<TextComponent>(window, "REINICIAR O SISTEMA", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
			s->addRow(row);

			row.elements.clear();
			row.makeAcceptInputHandler([window] {
				window->pushGui(new GuiMsgBox(window, "DESEJA DESLIGAR?", "SIM", 
				[] { 
					if(quitES("/tmp/es-shutdown") != 0)
						LOG(LogWarning) << "Shutdown terminated with non-zero result!";
				}, "NAO", nullptr));
			});
			row.addElement(std::make_shared<TextComponent>(window, "DESLIGAR O SISTEMA", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
			s->addRow(row);

		//	if(Settings::getInstance()->getBool("ShowExit"))
		//	{
		//		row.elements.clear();
		//		row.makeAcceptInputHandler([window] {
		//			window->pushGui(new GuiMsgBox(window, "REALLY QUIT?", "YES", 
		//			[] { 
		//				SDL_Event ev;
		//				ev.type = SDL_QUIT;
		//				SDL_PushEvent(&ev);
		//			}, "NO", nullptr));
		//		});
		//		row.addElement(std::make_shared<TextComponent>(window, "QUIT EMULATIONSTATION", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
		//		s->addRow(row);
		//	}

			mWindow->pushGui(s);
	});

	mVersion.setFont(Font::get(FONT_SIZE_SMALL));
	mVersion.setColor(0xC6C6C6FF);
	mVersion.setText("EMULATIONSTATION V" + strToUpper(PROGRAM_VERSION_STRING));
	mVersion.setAlignment(ALIGN_CENTER);

	addChild(&mMenu);
	addChild(&mVersion);

	setSize(mMenu.getSize());
	setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, Renderer::getScreenHeight() * 0.15f);
}

void GuiMenu::onSizeChanged()
{
	mVersion.setSize(mSize.x(), 0);
	mVersion.setPosition(0, mSize.y() - mVersion.getSize().y());
}

void GuiMenu::addEntry(const char* name, unsigned int color, bool add_arrow, const std::function<void()>& func)
{
	std::shared_ptr<Font> font = Font::get(FONT_SIZE_MEDIUM);
	
	// populate the list
	ComponentListRow row;
	row.addElement(std::make_shared<TextComponent>(mWindow, name, font, color), true);

	if(add_arrow)
	{
		std::shared_ptr<ImageComponent> bracket = makeArrow(mWindow);
		row.addElement(bracket, false);
	}
	
	row.makeAcceptInputHandler(func);

	mMenu.addRow(row);
}

bool GuiMenu::input(InputConfig* config, Input input)
{
	if(GuiComponent::input(config, input))
		return true;

	if((config->isMappedTo("b", input) || config->isMappedTo("start", input)) && input.value != 0)
	{
		delete this;
		return true;
	}

	return false;
}

std::vector<HelpPrompt> GuiMenu::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	prompts.push_back(HelpPrompt("up/down", "escolher"));
	prompts.push_back(HelpPrompt("a", "selecionar"));
	prompts.push_back(HelpPrompt("start", "fechar"));
	return prompts;
}
