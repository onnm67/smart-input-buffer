#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

using namespace geode::prelude;

// Only modes where a single click = a single jump benefit from buffering.
static bool isClickMode(PlayerObject* p) {
    if (!p) return false;
    return !(p->m_isShip || p->m_isBird || p->m_isDart || p->m_isSwing || p->m_isRobot);
}

class $modify(BufferLayer, GJBaseGameLayer) {
    struct Fields {
        int64_t tick = 0;                        // physics ticks since level start
        bool pending[2] = {false, false};        // buffered click waiting for ground
        int64_t pendingTick[2] = {0, 0};         // tick on which it was pressed
        bool physicallyHeld[2] = {false, false};
        bool releaseNextTick[2] = {false, false};
    };

    void handleButton(bool down, int button, bool isPlayer1) {
        auto& f = *m_fields.self();
        int i = isPlayer1 ? 0 : 1;

        if (button == 1) f.physicallyHeld[i] = down;

        if (Mod::get()->getSettingValue<bool>("enabled") && button == 1 && down) {
            auto* p = isPlayer1 ? m_player1 : m_player2;
            if (p && isClickMode(p) && !p->m_isOnGround) {
                f.pending[i] = true;
                f.pendingTick[i] = f.tick;
            }
        }

        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    // Runs once per physics tick, so the window is counted in ticks, not time.
    void processCommands(float dt) {
        auto& f = *m_fields.self();
        f.tick++;

        if (Mod::get()->getSettingValue<bool>("enabled")) {
            int64_t window = Mod::get()->getSettingValue<int64_t>("window-ticks");

            for (int i = 0; i < 2; i++) {
                auto* p = (i == 0) ? m_player1 : m_player2;
                bool p1 = (i == 0);

                if (f.releaseNextTick[i]) {
                    f.releaseNextTick[i] = false;
                    GJBaseGameLayer::handleButton(false, 1, p1);
                }

                if (!f.pending[i] || !p) continue;

                // Too old: drop it.
                if (f.tick - f.pendingTick[i] > window) {
                    f.pending[i] = false;
                    continue;
                }

                // Landed within the window: fire the click on this exact tick.
                if (p->m_isOnGround && isClickMode(p)) {
                    f.pending[i] = false;
                    GJBaseGameLayer::handleButton(false, 1, p1);
                    GJBaseGameLayer::handleButton(true, 1, p1);
                    if (!f.physicallyHeld[i]) f.releaseNextTick[i] = true;
                }
            }
        } else {
            f.pending[0] = f.pending[1] = false;
        }

        GJBaseGameLayer::processCommands(dt);
    }
};
