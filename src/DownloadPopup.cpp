#include "DownloadPopup.hpp"
#include <Geode/utils/web.hpp>
#include <fstream>
#include <thread>

using namespace geode::utils::web;

static std::string s_vkToken = "";

static std::string urlEncode(std::string const& s) {
    std::string out;
    for (char c : s) {
        if (isalnum(c) || c=='-' || c=='_' ||
            c=='.' || c=='~') out += c;
        else if (c==' ') out += '+';
        else {
            char h[4];
            snprintf(h, sizeof(h),
                "%%%02X", (unsigned char)c);
            out += h;
        }
    }
    return out;
}

static std::string htmlDecode(std::string const& s) {
    std::string out = s;
    size_t pos;
    while ((pos = out.find("&amp;")) != std::string::npos)
        out.replace(pos, 5, "&");
    while ((pos = out.find("&lt;")) != std::string::npos)
        out.replace(pos, 4, "<");
    while ((pos = out.find("&gt;")) != std::string::npos)
        out.replace(pos, 4, ">");
    while ((pos = out.find("&quot;")) != std::string::npos)
        out.replace(pos, 6, "\"");
    while ((pos = out.find("&#39;")) != std::string::npos)
        out.replace(pos, 5, "'");
    return out;
}

void DownloadPopup::setStatus(
    const char* text, ccColor3B color
) {
    if (m_closed) return;
    if (!m_statusLabel) return;
    if (!m_statusLabel->getParent()) return;
    m_statusLabel->setString(text);
    m_statusLabel->setColor(color);
}

DownloadPopup* DownloadPopup::create() {
    auto ret = new DownloadPopup();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

void DownloadPopup::switchTab(Tab tab) {
    m_currentTab = tab;

    if (m_searchPanel) m_searchPanel->setVisible(false);
    if (m_urlPanel) m_urlPanel->setVisible(false);
    if (m_vkPanel) m_vkPanel->setVisible(false);
    if (m_gdThemePanel) m_gdThemePanel->setVisible(false);
    if (m_skySoundPanel) m_skySoundPanel->setVisible(false);

    auto resetTab = [](CCMenuItemSpriteExtra* b) {
        if (b) b->setColor({160, 160, 160});
    };
    resetTab(m_tabSearch);
    resetTab(m_tabURL);
    resetTab(m_tabVK);
    resetTab(m_tabGDTheme);
    resetTab(m_tabSkySound);

    CCNode* panel = nullptr;
    CCMenuItemSpriteExtra* active = nullptr;

    switch (tab) {
        case Tab::Search:    panel = m_searchPanel;   active = m_tabSearch;   break;
        case Tab::DirectURL: panel = m_urlPanel;      active = m_tabURL;      break;
        case Tab::VKMusic:   panel = m_vkPanel;       active = m_tabVK;       break;
        case Tab::GDTheme:   panel = m_gdThemePanel;  active = m_tabGDTheme;  break;
        case Tab::SkySound:  panel = m_skySoundPanel; active = m_tabSkySound; break;
    }

    if (panel) panel->setVisible(true);
    if (active) active->setColor({255, 255, 255});
}

void DownloadPopup::onTabSearch(CCObject*)   { switchTab(Tab::Search); }
void DownloadPopup::onTabURL(CCObject*)      { switchTab(Tab::DirectURL); }
void DownloadPopup::onTabVK(CCObject*)       { switchTab(Tab::VKMusic); }
void DownloadPopup::onTabGDTheme(CCObject*)  { switchTab(Tab::GDTheme); }
void DownloadPopup::onTabSkySound(CCObject*) { switchTab(Tab::SkySound); }

bool DownloadPopup::init() {
    if (!FLAlertLayer::init(0)) return false;

    auto ws = CCDirector::sharedDirector()->getWinSize();
    float cx = ws.width / 2.f;
    float cy = ws.height / 2.f;

    auto bg = CCScale9Sprite::create("GJ_square01.png", {0, 0, 80, 80});
    bg->setContentSize({510.f, 330.f});
    bg->setPosition(ws / 2.f);
    m_mainLayer->addChild(bg);

    auto title = CCLabelBMFont::create("Music Downloader", "bigFont.fnt");
    title->setScale(0.68f);
    title->setPosition({cx, cy + 143.f});
    m_mainLayer->addChild(title);

    auto tabMenu = CCMenu::create();
    tabMenu->setPosition({0, 0});
    m_mainLayer->addChild(tabMenu, 5);

    auto makeTab = [&](const char* label, float x, float y,
        SEL_MenuHandler sel) -> CCMenuItemSpriteExtra* {
        auto spr = ButtonSprite::create(label, "chatFont.fnt", "GJ_button_04.png", 0.8f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, sel);
        btn->setPosition({x, y});
        btn->setScale(0.62f);
        btn->setColor({160, 160, 160});
        tabMenu->addChild(btn);
        return btn;
    };

    float tabY = cy + 115.f;
    m_tabSearch   = makeTab("Search", cx-170.f, tabY, menu_selector(DownloadPopup::onTabSearch));
    m_tabURL      = makeTab("URL",    cx-85.f,  tabY, menu_selector(DownloadPopup::onTabURL));
    m_tabVK       = makeTab("VK",     cx,       tabY, menu_selector(DownloadPopup::onTabVK));
    m_tabGDTheme  = makeTab("GD",     cx+85.f,  tabY, menu_selector(DownloadPopup::onTabGDTheme));
    m_tabSkySound = makeTab("Sky",    cx+170.f, tabY, menu_selector(DownloadPopup::onTabSkySound));

    auto idLabel = CCLabelBMFont::create("Save as Song ID:", "bigFont.fnt");
    idLabel->setScale(0.42f);
    idLabel->setPosition({cx, cy - 95.f});
    m_mainLayer->addChild(idLabel);

    m_songIdInput = TextInput::create(180.f, "e.g. 999999");
    m_songIdInput->setPosition({cx, cy - 115.f});
    m_songIdInput->setFilter("0123456789");
    m_songIdInput->setMaxCharCount(10);
    m_mainLayer->addChild(m_songIdInput);

    m_statusLabel = CCLabelBMFont::create("", "chatFont.fnt");
    m_statusLabel->setPosition({cx, cy - 143.f});
    m_statusLabel->setScale(0.45f);
    m_mainLayer->addChild(m_statusLabel);

    auto closeMenu = CCMenu::create();
    closeMenu->setPosition({0, 0});
    m_mainLayer->addChild(closeMenu, 10);

    auto cs = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
    cs->setScale(0.8f);
    auto cb = CCMenuItemSpriteExtra::create(cs, this, menu_selector(DownloadPopup::onClose));
    cb->setPosition({cx - 245.f, cy + 153.f});
    closeMenu->addChild(cb);

    // ══ Search Panel ══
    m_searchPanel = CCNode::create();
    m_searchPanel->setPosition({0, 0});
    m_mainLayer->addChild(m_searchPanel);

    auto sh = CCLabelBMFont::create("Search Deezer (30s) or GD Song ID:", "chatFont.fnt");
    sh->setScale(0.37f);
    sh->setPosition({cx, cy + 82.f});
    sh->setColor({200, 200, 200});
    m_searchPanel->addChild(sh);

    m_searchInput = TextInput::create(420.f, "Song name or GD Song ID...");
    m_searchInput->setPosition({cx, cy + 60.f});
    m_searchPanel->addChild(m_searchInput);

    m_songInfoLabel = CCLabelBMFont::create("Type and press Search", "chatFont.fnt");
    m_songInfoLabel->setPosition({cx, cy + 30.f});
    m_songInfoLabel->setScale(0.42f);
    m_songInfoLabel->setColor({180, 180, 180});
    m_searchPanel->addChild(m_songInfoLabel);

    auto searchMenu = CCMenu::create();
    searchMenu->setPosition({0, 0});
    m_searchPanel->addChild(searchMenu);

    auto prevS = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    prevS->setScale(0.55f);
    auto prevB = CCMenuItemSpriteExtra::create(prevS, this, menu_selector(DownloadPopup::onPrev));
    prevB->setPosition({cx - 220.f, cy + 30.f});
    searchMenu->addChild(prevB);

    auto nextS = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    nextS->setScale(0.55f);
    nextS->setFlipX(true);
    auto nextB = CCMenuItemSpriteExtra::create(nextS, this, menu_selector(DownloadPopup::onNext));
    nextB->setPosition({cx + 220.f, cy + 30.f});
    searchMenu->addChild(nextB);

    auto searchBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Search", "goldFont.fnt", "GJ_button_02.png", 0.8f),
        this, menu_selector(DownloadPopup::onSearch));
    searchBtn->setPosition({cx - 65.f, cy - 18.f});
    searchBtn->setScale(0.75f);
    searchMenu->addChild(searchBtn);

    auto dlBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Download", "goldFont.fnt", "GJ_button_01.png", 0.8f),
        this, menu_selector(DownloadPopup::onDownload));
    dlBtn->setPosition({cx + 65.f, cy - 18.f});
    dlBtn->setScale(0.75f);
    searchMenu->addChild(dlBtn);

    // ══ URL Panel ══
    m_urlPanel = CCNode::create();
    m_urlPanel->setPosition({0, 0});
    m_mainLayer->addChild(m_urlPanel);

    auto urlH = CCLabelBMFont::create("Paste any direct MP3/OGG link:", "chatFont.fnt");
    urlH->setScale(0.42f);
    urlH->setPosition({cx, cy + 82.f});
    urlH->setColor({255, 200, 100});
    m_urlPanel->addChild(urlH);

    m_urlDisplayLabel = CCLabelBMFont::create("No URL pasted yet", "chatFont.fnt");
    m_urlDisplayLabel->setPosition({cx, cy + 58.f});
    m_urlDisplayLabel->setScale(0.38f);
    m_urlDisplayLabel->setColor({180, 180, 180});
    m_urlPanel->addChild(m_urlDisplayLabel);

    auto urlH2 = CCLabelBMFont::create("catbox.moe | dropbox | discord | skysound", "chatFont.fnt");
    urlH2->setScale(0.35f);
    urlH2->setPosition({cx, cy + 33.f});
    urlH2->setOpacity(150);
    m_urlPanel->addChild(urlH2);

    auto urlMenu = CCMenu::create();
    urlMenu->setPosition({0, 0});
    m_urlPanel->addChild(urlMenu);

    auto pasteBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Paste URL", "goldFont.fnt", "GJ_button_04.png", 0.75f),
        this, menu_selector(DownloadPopup::onPaste));
    pasteBtn->setPosition({cx - 75.f, cy - 10.f});
    pasteBtn->setScale(0.75f);
    urlMenu->addChild(pasteBtn);

    auto dlUrlBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Download", "goldFont.fnt", "GJ_button_01.png", 0.75f),
        this, menu_selector(DownloadPopup::onDownloadURL));
    dlUrlBtn->setPosition({cx + 75.f, cy - 10.f});
    dlUrlBtn->setScale(0.75f);
    urlMenu->addChild(dlUrlBtn);

    // ══ VK Panel ══
    m_vkPanel = CCNode::create();
    m_vkPanel->setPosition({0, 0});
    m_mainLayer->addChild(m_vkPanel);

    auto vkH = CCLabelBMFont::create("VK Music (Kate Mobile token):", "chatFont.fnt");
    vkH->setScale(0.42f);
    vkH->setPosition({cx, cy + 85.f});
    vkH->setColor({100, 149, 237});
    m_vkPanel->addChild(vkH);

    m_vkTokenInput = TextInput::create(380.f, "Paste access_token...");
    m_vkTokenInput->setPosition({cx, cy + 62.f});
    m_vkPanel->addChild(m_vkTokenInput);
    if (!s_vkToken.empty()) m_vkTokenInput->setString(s_vkToken);

    auto vkSearchInput = TextInput::create(380.f, "Search in VK Music...");
    vkSearchInput->setPosition({cx, cy + 35.f});
    vkSearchInput->setTag(9999);
    m_vkPanel->addChild(vkSearchInput);

    auto vkMenu = CCMenu::create();
    vkMenu->setPosition({0, 0});
    m_vkPanel->addChild(vkMenu);

    auto getTokenBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Get Token", "goldFont.fnt", "GJ_button_03.png", 0.65f),
        this, menu_selector(DownloadPopup::onGetToken));
    getTokenBtn->setPosition({cx - 130.f, cy + 5.f});
    getTokenBtn->setScale(0.7f);
    vkMenu->addChild(getTokenBtn);

    auto pasteTokenBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Paste Token", "goldFont.fnt", "GJ_button_04.png", 0.65f),
        this, menu_selector(DownloadPopup::onPasteToken));
    pasteTokenBtn->setPosition({cx, cy + 5.f});
    pasteTokenBtn->setScale(0.7f);
    vkMenu->addChild(pasteTokenBtn);

    auto vkSearchBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Search", "goldFont.fnt", "GJ_button_02.png", 0.65f),
        this, menu_selector(DownloadPopup::onSearch));
    vkSearchBtn->setPosition({cx + 130.f, cy + 5.f});
    vkSearchBtn->setScale(0.7f);
    vkMenu->addChild(vkSearchBtn);

    auto vkDlBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Download", "goldFont.fnt", "GJ_button_01.png", 0.65f),
        this, menu_selector(DownloadPopup::onDownload));
    vkDlBtn->setPosition({cx, cy - 25.f});
    vkDlBtn->setScale(0.7f);
    vkMenu->addChild(vkDlBtn);

    // ══ GD Theme Panel ══
    m_gdThemePanel = CCNode::create();
    m_gdThemePanel->setPosition({0, 0});
    m_mainLayer->addChild(m_gdThemePanel);

    auto gdTitle2 = CCLabelBMFont::create("GD Official Soundtrack", "bigFont.fnt");
    gdTitle2->setScale(0.55f);
    gdTitle2->setPosition({cx, cy + 82.f});
    gdTitle2->setColor({255, 200, 50});
    m_gdThemePanel->addChild(gdTitle2);

    auto gdHint = CCLabelBMFont::create("Click song to download | Set Song ID below", "chatFont.fnt");
    gdHint->setScale(0.35f);
    gdHint->setPosition({cx, cy + 62.f});
    gdHint->setOpacity(180);
    m_gdThemePanel->addChild(gdHint);

    auto gdMenu = CCMenu::create();
    gdMenu->setPosition({0, 0});
    m_gdThemePanel->addChild(gdMenu);

    const char* songNames[] = {
        "Stereo Madness","Back On Track","Polargeist","Dry Out",
        "Base After Base","Cant Let Go","Jumper","Time Machine",
        "Cycles","xStep","Clutterfunk","Theory of Ev.",
        "Electroman Adv.","Clubstep","Electrodynamix"
    };
    int realNgIds[] = {
        128,155662,114192,126600,135880,141003,120194,123768,
        161480,130108,174180,187777,190553,202106,205226
    };

    for (int i = 0; i < 15; i++) {
        int row = i/5, col = i%5;
        float x = cx - 160.f + col * 80.f;
        float y = cy + 28.f - row * 28.f;
        auto songBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create(songNames[i], "chatFont.fnt", "GJ_button_04.png", 0.55f),
            this, menu_selector(DownloadPopup::onDownloadGDTheme));
        songBtn->setPosition({x, y});
        songBtn->setScale(0.6f);
        songBtn->setTag(realNgIds[i]);
        gdMenu->addChild(songBtn);
    }

    // ══ SkySound Panel ══
    m_skySoundPanel = CCNode::create();
    m_skySoundPanel->setPosition({0, 0});
    m_mainLayer->addChild(m_skySoundPanel);

    auto skyTitle = CCLabelBMFont::create("SkySound", "bigFont.fnt");
    skyTitle->setScale(0.7f);
    skyTitle->setPosition({cx, cy + 82.f});
    skyTitle->setColor({50, 200, 255});
    m_skySoundPanel->addChild(skyTitle);

    auto skySubtitle = CCLabelBMFont::create("Search full songs (no 30s limit)", "chatFont.fnt");
    skySubtitle->setScale(0.38f);
    skySubtitle->setPosition({cx, cy + 62.f});
    skySubtitle->setColor({150, 220, 255});
    m_skySoundPanel->addChild(skySubtitle);

    m_skySoundInput = TextInput::create(300.f, "Song name...");
    m_skySoundInput->setPosition({cx, cy + 40.f});
    m_skySoundPanel->addChild(m_skySoundInput);

    m_skyInfoLabel = CCLabelBMFont::create("Type and press Search", "chatFont.fnt");
    m_skyInfoLabel->setPosition({cx, cy + 10.f});
    m_skyInfoLabel->setScale(0.40f);
    m_skyInfoLabel->setColor({180, 180, 180});
    m_skySoundPanel->addChild(m_skyInfoLabel);

    auto skyMenu = CCMenu::create();
    skyMenu->setPosition({0, 0});
    m_skySoundPanel->addChild(skyMenu);

    auto skyPrevS = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    skyPrevS->setScale(0.55f);
    auto skyPrevB = CCMenuItemSpriteExtra::create(skyPrevS, this, menu_selector(DownloadPopup::onSkyPrev));
    skyPrevB->setPosition({cx - 220.f, cy + 10.f});
    skyMenu->addChild(skyPrevB);

    auto skyNextS = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    skyNextS->setScale(0.55f);
    skyNextS->setFlipX(true);
    auto skyNextB = CCMenuItemSpriteExtra::create(skyNextS, this, menu_selector(DownloadPopup::onSkyNext));
    skyNextB->setPosition({cx + 220.f, cy + 10.f});
    skyMenu->addChild(skyNextB);

    auto skySearchBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Search", "goldFont.fnt", "GJ_button_02.png", 0.8f),
        this, menu_selector(DownloadPopup::onSearchSkySound));
    skySearchBtn->setPosition({cx - 65.f, cy - 25.f});
    skySearchBtn->setScale(0.75f);
    skyMenu->addChild(skySearchBtn);

    auto skyDlBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Download", "goldFont.fnt", "GJ_button_01.png", 0.8f),
        this, menu_selector(DownloadPopup::onSkyDownload));
    skyDlBtn->setPosition({cx + 65.f, cy - 25.f});
    skyDlBtn->setScale(0.75f);
    skyMenu->addChild(skyDlBtn);

    auto skyHint = CCLabelBMFont::create("Full songs via sunproxy CDN", "chatFont.fnt");
    skyHint->setScale(0.28f);
    skyHint->setPosition({cx, cy - 55.f});
    skyHint->setOpacity(120);
    m_skySoundPanel->addChild(skyHint);

    m_urlPanel->setVisible(false);
    m_vkPanel->setVisible(false);
    m_gdThemePanel->setVisible(false);
    m_skySoundPanel->setVisible(false);
    switchTab(Tab::Search);

    setTouchEnabled(true);
    setKeypadEnabled(true);
    return true;
}

void DownloadPopup::show() { FLAlertLayer::show(); }

void DownloadPopup::registerWithTouchDispatcher() {
    CCDirector::sharedDirector()->getTouchDispatcher()
        ->addTargetedDelegate(this, -500, true);
}

void DownloadPopup::keyBackClicked() { onClose(nullptr); }

void DownloadPopup::onClose(CCObject*) {
    m_closed = true;
    m_downloading = false;
    m_searching = false;
    m_skySearching = false;
    if (m_vkTokenInput) s_vkToken = m_vkTokenInput->getString();
    setKeypadEnabled(false);
    removeFromParentAndCleanup(true);
}

void DownloadPopup::showCurrentResult() {
    if (m_closed) return;
    if (!m_songInfoLabel) return;
    if (m_results.empty()) {
        m_songInfoLabel->setString("No results");
        m_songInfoLabel->setColor({255, 80, 80});
        return;
    }
    SongResult& s = m_results[m_currentResult];
    std::string info;
    if (!s.artist.empty()) info = s.artist + " - " + s.title;
    else info = s.title;
    if (info.size() > 45) info = info.substr(0, 42) + "...";
    char buf[256];
    snprintf(buf, sizeof(buf), "[%d/%d] %s",
        m_currentResult + 1, (int)m_results.size(), info.c_str());
    m_songInfoLabel->setString(buf);
    m_songInfoLabel->setColor({100, 255, 100});
    if (m_songIdInput && m_songIdInput->getString().empty())
        m_songIdInput->setString(std::to_string(s.id));
}

void DownloadPopup::showSkyResult() {
    if (m_closed) return;
    if (!m_skyInfoLabel) return;
    if (m_skyResults.empty()) {
        m_skyInfoLabel->setString("No results");
        m_skyInfoLabel->setColor({255, 80, 80});
        return;
    }
    SongResult& s = m_skyResults[m_skyCurrentResult];
    if (s.title == "OPEN_BROWSER") {
        m_skyInfoLabel->setString("Press Search to open browser");
        m_skyInfoLabel->setColor({255, 200, 80});
        return;
    }
    std::string info;
    if (!s.artist.empty() && s.artist != "SkySound")
        info = s.artist + " - " + s.title;
    else info = s.title;
    if (info.size() > 45) info = info.substr(0, 42) + "...";
    char buf[256];
    snprintf(buf, sizeof(buf), "[%d/%d] %s",
        m_skyCurrentResult + 1, (int)m_skyResults.size(), info.c_str());
    m_skyInfoLabel->setString(buf);
    m_skyInfoLabel->setColor({100, 255, 100});
}

void DownloadPopup::onNext(CCObject*) {
    if (m_results.empty()) return;
    m_currentResult = (m_currentResult + 1) % (int)m_results.size();
    if (m_songIdInput) m_songIdInput->setString("");
    showCurrentResult();
}

void DownloadPopup::onPrev(CCObject*) {
    if (m_results.empty()) return;
    m_currentResult = (m_currentResult - 1 + (int)m_results.size()) % (int)m_results.size();
    if (m_songIdInput) m_songIdInput->setString("");
    showCurrentResult();
}

void DownloadPopup::onSkyNext(CCObject*) {
    if (m_skyResults.empty()) return;
    m_skyCurrentResult = (m_skyCurrentResult + 1) % (int)m_skyResults.size();
    showSkyResult();
}

void DownloadPopup::onSkyPrev(CCObject*) {
    if (m_skyResults.empty()) return;
    m_skyCurrentResult = (m_skyCurrentResult - 1 + (int)m_skyResults.size()) % (int)m_skyResults.size();
    showSkyResult();
}

void DownloadPopup::onSearchSkySound(CCObject*) {
    if (!m_skyResults.empty() && m_skyResults[0].title == "OPEN_BROWSER") {
        std::string url = m_skyResults[0].downloadUrl;
        m_skyResults.clear();
        utils::web::openLinkInBrowser(url);
        setStatus("Browser opened!", {50, 200, 255});
        if (!m_closed && m_skyInfoLabel) {
            m_skyInfoLabel->setString("Copy link -> URL tab -> Download");
            m_skyInfoLabel->setColor({50, 200, 255});
        }
        return;
    }
    if (m_skySearching) { setStatus("Already searching...", {255, 200, 80}); return; }
    std::string query = m_skySoundInput ? m_skySoundInput->getString() : "";
    if (query.empty()) { setStatus("Enter song name!", {255, 80, 80}); return; }
    m_skyResults.clear();
    m_skyCurrentResult = 0;
    m_skySearching = true;
    if (!m_closed && m_skyInfoLabel) {
        m_skyInfoLabel->setString("Searching...");
        m_skyInfoLabel->setColor({255, 255, 100});
    }
    setStatus("Searching SkySound...", {50, 200, 255});
    searchSkySound(query);
}

void DownloadPopup::onSkyDownload(CCObject*) {
    if (m_downloading) return;
    if (m_skyResults.empty()) { setStatus("Search first!", {255, 80, 80}); return; }
    SongResult& sr = m_skyResults[m_skyCurrentResult];
    if (sr.title == "OPEN_BROWSER") {
        utils::web::openLinkInBrowser(sr.downloadUrl);
        setStatus("Browser opened!", {50, 200, 255});
        return;
    }
    std::string idStr = m_songIdInput ? m_songIdInput->getString() : "";
    if (idStr.empty()) { setStatus("Enter Song ID!", {255, 80, 80}); return; }
    int songId = 0;
    try { songId = std::stoi(idStr); } catch (...) { return; }
    if (!sr.downloadUrl.empty()) doDownload(sr.downloadUrl, songId);
}

void DownloadPopup::searchSkySound(std::string const& query) {
    std::string slug;
    for (char c : query) {
        if (isalnum(c)) slug += tolower(c);
        else if (c == ' ' || c == '-') {
            if (!slug.empty() && slug.back() != '-') slug += '-';
        }
    }
    while (!slug.empty() && slug.back() == '-') slug.pop_back();

    std::string pageUrl = "https://" + slug + ".skysound7.com/";
    log::info("SkySound page: {}", pageUrl);

    std::thread([this, pageUrl, query, slug]() {
        WebRequest req;
        req.header("User-Agent",
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
            "AppleWebKit/537.36 (KHTML, like Gecko) "
            "Chrome/120.0.0.0 Safari/537.36");
        req.header("Accept", "text/html,application/xhtml+xml,*/*");
        req.header("Accept-Language", "ru-RU,ru;q=0.9,en;q=0.8");
        req.timeout(std::chrono::seconds(20));
        req.certVerification(false);
        req.followRedirects(true);

        WebResponse response = req.getSync(pageUrl);
        bool ok = response.ok();
        int code = response.code();
        std::string body;
        if (ok) { auto s = response.string(); if (s) body = s.unwrap(); }

        Loader::get()->queueInMainThread(
        [this, ok, code, body, query, pageUrl, slug]() {
            if (m_closed) { m_skySearching = false; return; }
            m_skySearching = false;

            if (!ok || body.empty()) {
                char buf[64];
                snprintf(buf, sizeof(buf), "SkySound error: HTTP %d", code);
                setStatus(buf, {255, 80, 80});
                if (m_skyInfoLabel) {
                    m_skyInfoLabel->setString("Connection error");
                    m_skyInfoLabel->setColor({255, 80, 80});
                }
                return;
            }

            log::info("SkySound HTML size: {}", body.size());
            std::vector<SongResult> found;

            auto parseFilename = [](const std::string& fullUrl,
                const std::string& fallback,
                std::string& outTitle, std::string& outArtist)
            {
                outTitle = fallback; outArtist = "";
                size_t lastSlash = fullUrl.rfind('/');
                if (lastSlash == std::string::npos) return;
                std::string filename = fullUrl.substr(lastSlash + 1);
                std::string decoded;
                for (size_t i = 0; i < filename.size(); i++) {
                    if (filename[i] == '%' && i + 2 < filename.size()) {
                        int val;
                        if (sscanf(filename.c_str()+i+1, "%2x", &val)==1) {
                            decoded += (char)val; i += 2; continue;
                        }
                    }
                    decoded += filename[i];
                }
                filename = decoded;
                size_t dot = filename.rfind('.');
                if (dot != std::string::npos) filename = filename.substr(0, dot);
                size_t paren = filename.find('(');
                if (paren != std::string::npos) filename = filename.substr(0, paren);
                for (auto& c : filename) if (c == '_') c = ' ';
                while (!filename.empty() && filename.back()==' ') filename.pop_back();
                while (!filename.empty() && filename[0]==' ') filename = filename.substr(1);
                if (filename.empty()) return;
                size_t sep = filename.find(" - ");
                if (sep != std::string::npos) {
                    outArtist = filename.substr(0, sep);
                    outTitle = filename.substr(sep + 3);
                    while (outTitle.find("- ")==0) outTitle = outTitle.substr(2);
                    while (outTitle.find(" -")==0) outTitle = outTitle.substr(2);
                    while (!outTitle.empty() && outTitle[0]==' ') outTitle = outTitle.substr(1);
                    while (!outArtist.empty() && outArtist.back()==' ') outArtist.pop_back();
                } else { outTitle = filename; }
            };

            auto addResult = [&](const std::string& url) {
                std::string urlLower = url;
                for (auto& c : urlLower) c = tolower(c);
                bool isAudio =
                    urlLower.find("sunproxy.net") != std::string::npos ||
                    (urlLower.find(".mp3") != std::string::npos &&
                     urlLower.find("http") == 0);
                if (!isAudio) return;
                if (url.find("javascript") != std::string::npos) return;
                if (url.size() < 10) return;
                std::string fullUrl = htmlDecode(url);
                for (auto& f : found)
                    if (f.downloadUrl == fullUrl) return;
                std::string title, artist;
                parseFilename(fullUrl, query, title, artist);
                SongResult sr;
                sr.title = title;
                sr.artist = artist.empty() ? "SkySound" : artist;
                sr.downloadUrl = fullUrl;
                sr.id = 0;
                found.push_back(sr);
                log::info("Found: {} - {} -> {}", sr.artist, sr.title, fullUrl);
            };

            const char* attrs[] = {
                "href","data-url","data-src","data-href","src",nullptr
            };
            for (int a = 0; attrs[a]; a++) {
                std::string pattern = std::string(attrs[a]) + "=\"";
                size_t pos = 0;
                while (pos < body.size()) {
                    size_t attrPos = body.find(pattern, pos);
                    if (attrPos == std::string::npos) break;
                    attrPos += pattern.size();
                    size_t endQuote = body.find("\"", attrPos);
                    if (endQuote == std::string::npos) break;
                    std::string url = body.substr(attrPos, endQuote - attrPos);
                    pos = endQuote + 1;
                    addResult(url);
                }
            }

            m_skyResults = found;
            m_skyCurrentResult = 0;

            if (m_skyResults.empty()) {
                setStatus("No audio - press Search for browser", {255, 200, 80});
                if (m_skyInfoLabel) {
                    m_skyInfoLabel->setString("Press Search again for browser");
                    m_skyInfoLabel->setColor({255, 200, 80});
                }
                SongResult placeholder;
                placeholder.title = "OPEN_BROWSER";
                placeholder.downloadUrl = pageUrl;
                m_skyResults.push_back(placeholder);
                return;
            }

            char buf[64];
            snprintf(buf, sizeof(buf), "SkySound: %d songs!", (int)m_skyResults.size());
            setStatus(buf, {50, 200, 255});
            showSkyResult();
        });
    }).detach();
}

void DownloadPopup::onPaste(CCObject*) {
    std::string clip = utils::clipboard::read();
    if (clip.empty()) { setStatus("Clipboard empty!", {255, 80, 80}); return; }
    m_rawUrl = clip;
    if (!m_closed && m_urlDisplayLabel) {
        std::string display = clip;
        if (display.size() > 60) display = display.substr(0, 57) + "...";
        m_urlDisplayLabel->setString(display.c_str());
        m_urlDisplayLabel->setColor({100, 255, 100});
    }
    log::info("Pasted raw URL: {}", m_rawUrl);
    setStatus("URL pasted!", {200, 200, 255});
}

void DownloadPopup::onPasteToken(CCObject*) {
    std::string clip = utils::clipboard::read();
    if (clip.empty()) return;
    std::string token = clip;
    auto pos = clip.find("access_token=");
    if (pos != std::string::npos) {
        pos += 13;
        auto end = clip.find('&', pos);
        token = (end != std::string::npos) ? clip.substr(pos, end-pos) : clip.substr(pos);
    }
    if (m_vkTokenInput) m_vkTokenInput->setString(token);
    s_vkToken = token;
    setStatus("VK token set!", {100, 149, 237});
}

void DownloadPopup::onGetToken(CCObject*) {
    utils::web::openLinkInBrowser(
        "https://oauth.vk.com/authorize?client_id=2685278"
        "&scope=audio,offline&redirect_uri=https://oauth.vk.com/blank.html"
        "&display=page&response_type=token");
    FLAlertLayer::create(nullptr, "Get VK Token",
        "1. Login to VK\n2. Click <cg>Allow</c>\n"
        "3. Copy <cy>full URL</c> from address bar\n"
        "4. Press <cg>Paste Token</c>",
        "OK", nullptr, 360.f)->show();
    setStatus("Browser opened!", {100, 149, 237});
}

void DownloadPopup::onDownloadURL(CCObject*) {
    if (m_downloading) return;
    std::string url = m_rawUrl;
    std::string idStr = m_songIdInput ? m_songIdInput->getString() : "";
    if (url.empty() || url.find("http") != 0) { setStatus("Paste URL first!", {255, 80, 80}); return; }
    if (idStr.empty()) { setStatus("Enter Song ID!", {255, 80, 80}); return; }
    int songId = 0;
    try { songId = std::stoi(idStr); } catch (...) { return; }
    doDownload(url, songId);
}

void DownloadPopup::onDownloadGDTheme(CCObject* sender) {
    auto btn = dynamic_cast<CCNode*>(sender);
    if (!btn) return;
    int ngId = btn->getTag();
    if (ngId <= 0) return;
    int songId = ngId;
    if (m_songIdInput && !m_songIdInput->getString().empty()) {
        try { songId = std::stoi(m_songIdInput->getString()); } catch (...) { songId = ngId; }
    }
    std::string url = "https://audio.ngfiles.com/" +
        std::to_string(ngId/1000*1000) + "/" + std::to_string(ngId) + "_full.mp3";
    char buf[64];
    snprintf(buf, sizeof(buf), "Downloading GD song #%d...", ngId);
    setStatus(buf, {255, 200, 50});
    doDownload(url, songId);
}

void DownloadPopup::onSearch(CCObject*) {
    if (m_searching) { setStatus("Already searching...", {255, 200, 80}); return; }
    std::string query = "";
    if (m_currentTab == Tab::VKMusic) {
        auto vkInput = dynamic_cast<TextInput*>(m_vkPanel->getChildByTag(9999));
        if (vkInput) query = vkInput->getString();
    } else {
        if (m_searchInput) query = m_searchInput->getString();
    }
    if (query.empty()) { setStatus("Enter song name or ID!", {255, 80, 80}); return; }
    m_results.clear();
    m_currentResult = 0;
    m_searching = true;
    if (m_vkTokenInput) s_vkToken = m_vkTokenInput->getString();
    bool isNumber = true;
    for (char c : query) if (!isdigit(c)) { isNumber = false; break; }
    if (isNumber) { setStatus("Fetching GD song...", {255, 255, 100}); searchGDBrowser(query); }
    else if (!s_vkToken.empty() && m_currentTab == Tab::VKMusic) { setStatus("Searching VK...", {100, 149, 237}); searchVK(query); }
    else { setStatus("Searching Deezer...", {255, 255, 100}); searchDeezer(query); }
}

void DownloadPopup::searchGDBrowser(std::string const& songIdStr) {
    std::thread([this, songIdStr]() {
        WebRequest req;
        req.header("User-Agent", "GeodeMod/1.0");
        req.timeout(std::chrono::seconds(10));
        req.certVerification(false);
        req.followRedirects(true);
        WebResponse response = req.getSync("https://gdbrowser.com/api/song?id=" + songIdStr);
        bool ok = response.ok();
        std::string body;
        if (ok) { auto s = response.string(); if (s) body = s.unwrap(); }

        Loader::get()->queueInMainThread([this, ok, body, songIdStr]() {
            if (m_closed) { m_searching = false; return; }
            m_searching = false;
            int ngId = 0;
            try { ngId = std::stoi(songIdStr); } catch (...) {}
            if (ok && !body.empty()) {
                auto pr = matjson::parse(body);
                if (pr) {
                    auto json = pr.unwrap(); SongResult sr; sr.id = ngId;
                    if (json.contains("name")) { auto v = json["name"].asString(); if (v) sr.title = v.unwrap() + " [GD Full]"; }
                    if (json.contains("author")) { auto v = json["author"].asString(); if (v) sr.artist = v.unwrap(); }
                    if (json.contains("download")) { auto v = json["download"].asString(); if (v) sr.downloadUrl = v.unwrap(); }
                    if (!sr.downloadUrl.empty()) {
                        if (sr.title.empty()) sr.title = "Song #" + songIdStr + " [GD]";
                        if (sr.artist.empty()) sr.artist = "Unknown";
                        m_results.push_back(sr);
                        setStatus("GD song found!", {80, 255, 80});
                        showCurrentResult(); return;
                    }
                }
            }
            if (ngId > 0) {
                SongResult sr; sr.id = ngId;
                sr.title = "Song #" + songIdStr + " [GD]";
                sr.artist = "Newgrounds";
                sr.downloadUrl = "https://audio.ngfiles.com/" +
                    std::to_string(ngId/1000*1000) + "/" + songIdStr + "_full.mp3";
                m_results.push_back(sr);
                setStatus("Fallback URL", {255, 200, 80});
                showCurrentResult(); return;
            }
            setStatus("Not found!", {255, 80, 80});
        });
    }).detach();
}

void DownloadPopup::searchVK(std::string const& query) {
    std::string token = s_vkToken;
    std::string enc = urlEncode(query);
    std::thread([this, token, enc, query]() {
        WebRequest req;
        req.header("User-Agent", "VKAndroidApp/5.52-4543 (Android 5.1.1; SDK 22; x86_64; unknown Android SDK built for x86_64; en; 320x240)");
        req.timeout(std::chrono::seconds(15));
        req.certVerification(false);
        req.followRedirects(true);
        WebResponse response = req.getSync("https://api.vk.com/method/audio.search?q=" + enc + "&count=10&access_token=" + token + "&v=5.131");
        bool ok = response.ok();
        std::string body;
        if (ok) { auto s = response.string(); if (s) body = s.unwrap(); }

        Loader::get()->queueInMainThread([this, ok, body, query]() {
            if (m_closed) { m_searching = false; return; }
            if (!ok || body.empty()) { m_searching = true; searchDeezer(query); return; }
            auto pr = matjson::parse(body);
            if (!pr) { m_searching = false; setStatus("Parse error!", {255, 80, 80}); return; }
            auto json = pr.unwrap();
            if (json.contains("error")) { m_searching = true; setStatus("VK token error!", {255, 80, 80}); searchDeezer(query); return; }
            if (!json.contains("response") || !json["response"].contains("items")) { m_searching = false; setStatus("No VK results!", {255, 80, 80}); return; }
            auto& items = json["response"]["items"];
            int i = 0;
            while (i < 10) {
                try {
                    auto& item = items[i]; SongResult sr;
                    if (item.contains("id")) { auto v = item["id"].asInt(); if (v) sr.id = v.unwrap(); }
                    if (item.contains("title")) { auto v = item["title"].asString(); if (v) sr.title = v.unwrap() + " [VK Full]"; }
                    if (item.contains("artist")) { auto v = item["artist"].asString(); if (v) sr.artist = v.unwrap(); }
                    if (item.contains("url")) { auto v = item["url"].asString(); if (v) sr.downloadUrl = v.unwrap(); }
                    if (!sr.downloadUrl.empty()) m_results.push_back(sr);
                    i++;
                } catch (...) { break; }
            }
            m_searching = false;
            if (m_results.empty()) { m_searching = true; searchDeezer(query); return; }
            char buf[64]; snprintf(buf, sizeof(buf), "VK: %d songs!", (int)m_results.size());
            setStatus(buf, {100, 149, 237}); showCurrentResult();
        });
    }).detach();
}

void DownloadPopup::searchDeezer(std::string const& query) {
    std::string enc = urlEncode(query);
    std::thread([this, enc]() {
        WebRequest req;
        req.header("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
        req.timeout(std::chrono::seconds(15));
        req.certVerification(false);
        req.followRedirects(true);
        WebResponse response = req.getSync("https://api.deezer.com/search?q=" + enc + "&limit=10&output=json");
        bool ok = response.ok();
        std::string body;
        if (ok) { auto s = response.string(); if (s) body = s.unwrap(); }

        Loader::get()->queueInMainThread([this, ok, body]() {
            if (m_closed) { m_searching = false; return; }
            m_searching = false;
            if (!ok || body.empty()) { setStatus("Deezer error!", {255, 80, 80}); return; }
            auto pr = matjson::parse(body);
            if (!pr || !pr.unwrap().contains("data")) { setStatus("No results!", {255, 80, 80}); return; }
            auto& data = pr.unwrap()["data"];
            int i = 0;
            while (i < 10) {
                try {
                    auto& item = data[i]; SongResult sr;
                    if (item.contains("id")) { auto v = item["id"].asInt(); if (v) sr.id = v.unwrap(); }
                    if (item.contains("title")) { auto v = item["title"].asString(); if (v) sr.title = v.unwrap() + " [30s]"; }
                    if (item.contains("artist") && item["artist"].contains("name")) { auto v = item["artist"]["name"].asString(); if (v) sr.artist = v.unwrap(); }
                    if (item.contains("preview")) { auto v = item["preview"].asString(); if (v) sr.downloadUrl = v.unwrap(); }
                    if (!sr.downloadUrl.empty() && sr.id > 0) m_results.push_back(sr);
                    i++;
                } catch (...) { break; }
            }
            if (m_results.empty()) { setStatus("No results!", {255, 80, 80}); return; }
            char buf[64]; snprintf(buf, sizeof(buf), "Deezer: %d songs!", (int)m_results.size());
            setStatus(buf, {80, 255, 80}); showCurrentResult();
        });
    }).detach();
}

void DownloadPopup::onDownload(CCObject*) {
    if (m_downloading || m_searching) return;
    if (m_results.empty()) { setStatus("Search first!", {255, 80, 80}); return; }
    std::string idStr = m_songIdInput ? m_songIdInput->getString() : "";
    if (idStr.empty()) { setStatus("Enter Song ID!", {255, 80, 80}); return; }
    int songId = 0;
    try { songId = std::stoi(idStr); } catch (...) { return; }
    SongResult& sr = m_results[m_currentResult];
    if (!sr.downloadUrl.empty()) doDownload(sr.downloadUrl, songId);
}

void DownloadPopup::doDownload(std::string const& url, int songId) {
    m_downloading = true;
    setStatus("Downloading...", {255, 255, 100});
    if (!m_closed && m_songInfoLabel) {
        m_songInfoLabel->setString("Downloading...");
        m_songInfoLabel->setColor({255, 255, 100});
    }
    if (!m_closed && m_skyInfoLabel && m_currentTab == Tab::SkySound) {
        m_skyInfoLabel->setString("Downloading...");
        m_skyInfoLabel->setColor({255, 255, 100});
    }

    std::string songPath = CCFileUtils::sharedFileUtils()->getWritablePath()
        + std::to_string(songId) + ".mp3";
    log::info("URL: {}", url);
    log::info("Path: {}", songPath);

    std::thread([this, url, songId, songPath]() {
        WebRequest req;
        req.header("User-Agent",
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
            "AppleWebKit/537.36 (KHTML, like Gecko) "
            "Chrome/120.0.0.0 Safari/537.36");
        req.header("Accept", "audio/mpeg,audio/*,application/octet-stream,*/*");
        if (url.find("skysound") != std::string::npos ||
            url.find("sunproxy") != std::string::npos)
            req.header("Referer", "https://sky-sond.skysound7.com/");
        req.timeout(std::chrono::seconds(180));
        req.certVerification(false);
        req.followRedirects(true);

        WebResponse response = req.getSync(url);
        bool ok = response.ok();
        int code = response.code();
        ByteVector fd = std::move(response).data();

        Loader::get()->queueInMainThread(
        [this, ok, code, fd = std::move(fd), songId, songPath, url]() mutable {
            if (m_closed) return;
            m_downloading = false;

            if (!ok) {
                char buf[64]; snprintf(buf, sizeof(buf), "Failed: HTTP %d", code);
                setStatus(buf, {255, 80, 80}); return;
            }
            if (fd.empty()) { setStatus("Empty file!", {255, 80, 80}); return; }

            bool isHTML = false;
            if (fd.size() > 15) {
                std::string head(reinterpret_cast<const char*>(fd.data()),
                    std::min(fd.size(), (size_t)512));
                std::string headLower = head;
                for (auto& c : headLower) c = tolower(c);
                if (headLower.find("<!doctype") != std::string::npos ||
                    headLower.find("<html") != std::string::npos ||
                    headLower.find("<head") != std::string::npos)
                    isHTML = true;
            }
            if (isHTML) {
                float kb = (float)fd.size() / 1024.f;
                char buf[256]; snprintf(buf, sizeof(buf), "Got HTML (%.1f KB) not audio!", kb);
                setStatus(buf, {255, 80, 80}); return;
            }

            bool isOGG = fd.size() > 4 &&
                fd[0]=='O' && fd[1]=='g' && fd[2]=='g' && fd[3]=='S';
            bool isMP3 = fd.size() > 3 &&
                ((fd[0]==0xFF && (fd[1]&0xE0)==0xE0) ||
                 (fd[0]=='I' && fd[1]=='D' && fd[2]=='3'));

            std::string finalPath = isOGG
                ? CCFileUtils::sharedFileUtils()->getWritablePath()
                  + std::to_string(songId) + ".ogg"
                : songPath;

            std::ofstream file(finalPath, std::ios::binary | std::ios::trunc);
            if (!file.is_open()) { setStatus("Cannot save!", {255, 80, 80}); return; }
            file.write(reinterpret_cast<const char*>(fd.data()), fd.size());
            file.close();

            MusicDownloadManager::sharedState()->songStateChanged();

            float mb = (float)fd.size() / (1024.f * 1024.f);
            char buf[128]; snprintf(buf, sizeof(buf), "Done! %.2f MB -> ID #%d", mb, songId);
            setStatus(buf, {80, 255, 80});

            if (!m_closed && m_songInfoLabel) {
                m_songInfoLabel->setString("Ready! Use Song ID in editor.");
                m_songInfoLabel->setColor({80, 255, 80});
            }
            if (!m_closed && m_skyInfoLabel && m_currentTab == Tab::SkySound) {
                m_skyInfoLabel->setString("Ready! Use Song ID in editor.");
                m_skyInfoLabel->setColor({80, 255, 80});
            }
            log::info("Saved {} ({:.2f}MB) fmt:{}", songId, mb,
                isOGG ? "OGG" : isMP3 ? "MP3" : "unknown");
        });
    }).detach();
}