#pragma once

struct UserCmd;
struct Vector;

namespace Legitbot
{
    void DoStandAloneRecoil(UserCmd*) noexcept;
    void run(UserCmd*) noexcept;
}