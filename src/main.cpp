#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/EditLevelLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include "DownloadPopup.hpp"

using namespace geode::prelude;

class $modify(MusicMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto spr = CCSprite::createWithSpriteFrameName(
            "GJ_downloadBtn_001.png");
        spr->setScale(0.75f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this,
            menu_selector(MusicMenuLayer::onOpenDownloader));
        btn->setID("music-downloader-btn"_spr);

        if (auto m = this->getChildByID("bottom-menu")) {
            auto bm = static_cast<CCMenu*>(m);
            bm->addChild(btn);
            bm->updateLayout();
        }
        return true;
    }

    void onOpenDownloader(CCObject*) {
        DownloadPopup::create()->show();
    }
};


class $modify(MyLevelInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge))
            return false;

        auto spr = CCSprite::createWithSpriteFrameName(
            "GJ_downloadBtn_001.png");
        spr->setScale(0.7f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this,
            menu_selector(MyLevelInfoLayer::onMusicDownloader));
        btn->setID("music-downloader-btn"_spr);

        if (auto m = this->getChildByID("left-side-menu")) {
            auto lm = static_cast<CCMenu*>(m);
            lm->addChild(btn);
            lm->updateLayout();
        } else {
            auto ws = CCDirector::sharedDirector()->getWinSize();
            auto dlMenu = CCMenu::create();
            dlMenu->setPosition({0, 0});
            btn->setPosition({70.f, 35.f});
            dlMenu->addChild(btn);
            this->addChild(dlMenu, 10);
        }
        return true;
    }

    void onMusicDownloader(CCObject*) {
        DownloadPopup::create()->show();
    }
};

class $modify(MyPlayLayer, PlayLayer) {
    void resetLevel() {
        FMODAudioEngine::sharedEngine()->stopAllMusic(true);
        PlayLayer::resetLevel();
    }
};