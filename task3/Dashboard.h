// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
class Engine;
namespace task3 {
// Must run before QApplication is constructed (Qt WebEngine shares OpenGL contexts).
void prepareWebEngine();
void installDashboard(Engine& engine);
}
