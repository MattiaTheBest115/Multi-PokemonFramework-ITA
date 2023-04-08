#include <CTRPluginFramework.hpp>
#include "stdafx.hpp"
#include "Helpers.hpp"

namespace CTRPluginFramework {
    u8 data8; u16 data16; u32 data32;

    string bin;
    string path;

    bool IsCompatible(void) {
        u32 titleID = Process::GetTitleID();
        u16 ver = Process::GetVersion();

        static const vector<u32> valids = {0x55D00, 0x55E00, 0x11C400, 0x11C500, 0x164800, 0x175E00, 0x1B5000, 0x1B5100};
        static const vector<Game> allGames = {Game::X, Game::Y, Game::OR, Game::AS, Game::S, Game::M, Game::US, Game::UM};
        static const vector<Group> allGroups = {Group::XY, Group::ORAS, Group::SM, Group::USUM};
        static const vector<u16> updates = {5232, 5216, 7820, 7820, 2112, 2112, 2080, 2080};

        for (unsigned int i = 0; i < valids.size(); i++) {
            if (titleID == valids[i]) {
                game = allGames[i];
                group = allGroups[(int)(i / 2)];

                if (ver == updates[i])
                    update = Update::Supported;

                bin = "/luma/plugins/00040000" + Utils::ToHex(titleID) + "/Bin/";
                path = "/luma/plugins/00040000" + Utils::ToHex(titleID);
                return true;
            }

            else {
                game = Game::None;
                group = Group::None;
            }
        }

        return false;
    }

    Lang currLang = Lang::ENG;

    void LangFile(Lang lang) {
        File file(path + "/Lang.txt");
        string language;

        if (File::Exists(path + "/Lang.txt") == 1)
            File::Remove(path + "/Lang.txt");

        if (File::Exists(path + "/Lang.txt") == 0)
            File::Create(path + "/Lang.txt");

        if (lang == Lang::ENG)
            language = "English";

        if (lang == Lang::FRE)
            language = "Francais";

        if (File::Exists(path + "/Lang.txt") == 1) {
            LineWriter writeFile(file);
            writeFile << language;
            writeFile.Flush();
            writeFile.Close();
            Message::Completed();
            Process::ReturnToHomeMenu();
        }
    }

    string language(string english, string french) {
        return (currLang == Lang::ENG ? english : french);
    }

    void Settings(MenuEntry *entry) {
        static const vector<string> options = {language("Language", "Langue"), language("Reset", "Réinitialiser")};
        KeyboardPlus keyboard;
        int settings;

        if (keyboard.SetKeyboard(entry->Name() + ":", true, options, settings) != -1) {
            if (settings == 0) {
                static const vector<string> langOption = {language("English", "Anglais"), language("French", "Français")};
                int chooseLang;

                if (keyboard.SetKeyboard(language("Language:\n\nNote: changing language will require a restart of the game!", "Langue:\n\nNote: changer la langue nécessitera un redémarrage du jeu!"), true, langOption, chooseLang) != -1) {
                    if (chooseLang == 0) {
                        LangFile(Lang::ENG);
                        return;
                    }

                    if (chooseLang == 1) {
                        LangFile(Lang::FRE);
                        return;
                    }
                }
            }

            if (settings == 1) {
                if (MessageBox(CenterAlign(language("Would you like to reset settings?", "Voulez-vous réinitialiser les paramètres?")), DialogType::DialogYesNo, ClearScreen::Both)()) {
                    if (File::Exists("Data.bin")) {
                        File::Remove("Data.bin");
                        Message::Completed();
                        Process::ReturnToHomeMenu();
                        return;
                    }

                    Message::Warning();
                }
            }
        }
    }

    namespace Message {
        void Completed(void) {
            MessageBox(CenterAlign(language("Operation has been ", "L'opération a été ") << Color::LimeGreen << language("completed", "terminée") << Color::White << "!"), DialogType::DialogOk, ClearScreen::Both)();
        }

        void Interrupted(void) {
            MessageBox(CenterAlign(language("Operation has been ", "L'opération a été ") << Color(255, 51, 51) << language("interrupted", "suspendu") << Color::White << "!"), DialogType::DialogOk, ClearScreen::Both)();
        }

        void Warning(void) {
            MessageBox(CenterAlign(language("Operation has already been ", "L'opération a déjà été ") << Color::Orange << language("completed", "accomplie") << Color::White << "!"), DialogType::DialogOk, ClearScreen::Both)();
        }
    }

    namespace Helpers {
        static int isColored = false, chosenColor;

        bool TextColorizer(u32 address) {
            ColoredText textColors[7] = {
                {language("Red", "Rouge"), 0},
                {language("Green", "Vert"), 1},
                {language("Blue", "Bleu"), 2},
                {"Orange", 3},
                {language("Pink", "Rose"), 4},
                {language("Purple", "Violet"), 5},
                {language("Light Blue", "Bleu Clair"), 6}
            };

            static const vector<string> options = {language("No", "Non"), language("Yes", "Oui")};
            KeyboardPlus keyboard;

            if (keyboard.SetKeyboard(language("Colored?", "Coloré?"), true, options, isColored) != -1) {
                if (isColored == 1) {
                    static vector<string> colors;

                    if (colors.empty()) {
                        for (ColoredText &nickname : textColors)
                            colors.push_back(nickname.name);
                    }

                    if (keyboard.SetKeyboard(language("Color:", "Coleur:"), true, colors, chosenColor) != -1) {
                        Process::Write32(address, 0x20010);
                        Process::Write16(address + 0x6, textColors[chosenColor].val);
                        Process::Write16(address + 0x4, 0xBD00);
                    }

                    return true;
                }

                else return false;
            }

            else return false;
        }

        namespace Battle {
            vector<u32> offset(2);
        }

        void MenuCallback(void) {
            Battle::offset = {
                Helpers::AutoRegion<u32>(
                    Helpers::GetVersion<u32>(
                        {0x81FB284, 0x81FB624},
                        {0x81FB58C, 0x81FB92C}),

                    {0x30000484, 0x3000746C}
            )};
        }
    }

    bool IsInBattle(void) {
        static const u32 pointer = Helpers::AutoRegion(Helpers::GetVersion(0x81FB170, 0x81FB478), 0x30000158);

        if (group != Group::SM && group != Group::USUM) {
            if (ProcessPlus::Read32(pointer) == 0x40001)
                return true;
        }

        else {
            if (ProcessPlus::Read32(pointer) == 0x40001) {
                if (ProcessPlus::Read8(pointer + 0x28) == 3)
                    return true;
            }
        }

        return false;
    }
}