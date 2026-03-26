#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

struct SongResult {
    std::string title;
    std::string artist;
    std::string downloadUrl;
    int id = 0;
};

enum class Tab {
    Search,
    DirectURL,
    VKMusic,
    GDTheme,
    SkySound
};

class DownloadPopup : public FLAlertLayer {
protected:
    TextInput* m_searchInput      = nullptr;
    TextInput* m_songIdInput      = nullptr;
    TextInput* m_vkTokenInput     = nullptr;
    TextInput* m_skySoundInput    = nullptr;

    CCLabelBMFont* m_statusLabel      = nullptr;
    CCLabelBMFont* m_songInfoLabel    = nullptr;
    CCLabelBMFont* m_urlDisplayLabel  = nullptr;
    CCLabelBMFont* m_skyInfoLabel     = nullptr;

    std::string m_rawUrl;

    std::vector<SongResult> m_skyResults;
    int m_skyCurrentResult = 0;
    bool m_skySearching = false;

    CCNode* m_searchPanel   = nullptr;
    CCNode* m_urlPanel      = nullptr;
    CCNode* m_vkPanel       = nullptr;
    CCNode* m_gdThemePanel  = nullptr;
    CCNode* m_skySoundPanel = nullptr;

    CCMenuItemSpriteExtra* m_tabSearch   = nullptr;
    CCMenuItemSpriteExtra* m_tabURL      = nullptr;
    CCMenuItemSpriteExtra* m_tabVK       = nullptr;
    CCMenuItemSpriteExtra* m_tabGDTheme  = nullptr;
    CCMenuItemSpriteExtra* m_tabSkySound = nullptr;

    Tab m_currentTab = Tab::Search;

    bool m_downloading = false;
    bool m_searching   = false;
    bool m_closed      = false;

    std::vector<SongResult> m_results;
    int m_currentResult = 0;

    bool init();

    void onTabSearch(CCObject*);
    void onTabURL(CCObject*);
    void onTabVK(CCObject*);
    void onTabGDTheme(CCObject*);
    void onTabSkySound(CCObject*);
    void switchTab(Tab tab);

    void onSearch(CCObject*);
    void onDownload(CCObject*);
    void onDownloadURL(CCObject*);
    void onPaste(CCObject*);
    void onPasteToken(CCObject*);
    void onGetToken(CCObject*);
    void onClose(CCObject*);
    void onNext(CCObject*);
    void onPrev(CCObject*);
    void onDownloadGDTheme(CCObject*);
    void onSearchSkySound(CCObject*);
    void onSkyPrev(CCObject*);
    void onSkyNext(CCObject*);
    void onSkyDownload(CCObject*);

    void searchGDBrowser(std::string const& id);
    void searchDeezer(std::string const& query);
    void searchVK(std::string const& query);
    void searchSkySound(std::string const& query);
    void showSkyResult();

    void doDownload(std::string const& url, int songId);
    void setStatus(const char* text, ccColor3B color);
    void showCurrentResult();

    void registerWithTouchDispatcher() override;
    void keyBackClicked() override;

public:
    static DownloadPopup* create();
    void show();
};