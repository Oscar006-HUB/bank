# Bank App

An ATM-style banking application with a graphical interface, written in **C** with **[raylib](https://www.raylib.com/)**.

## Features

- 🔐 **PIN login:** 4-digit PIN entry (masked), with a clear message on a wrong PIN
- 📊 **Account dashboard:** account holder, account number and live balance in GHS
- 💸 **Withdraw:** checks for empty, zero or negative amounts and insufficient funds
- 💰 **Deposit:** adds funds and shows the new balance
- 🔁 **Transfer:** send money to another account number, with validation
- ✅ **Confirmation screens** after every transaction
- 🎵 **Optional background music** (`assets/badbunny.mp3`)

## Screens

```
LOGIN ──▶ DASHBOARD ──┬──▶ WITHDRAW ──▶ WITHDRAWAL SUCCESSFUL ──┐
   ▲                  ├──▶ DEPOSIT  ──▶ DEPOSIT SUCCESSFUL    ──┤
   │                  ├──▶ TRANSFER ──▶ TRANSFER SUCCESSFUL   ──┤
   └──── Logout ──────┘                                          │
                      ▲────────────── Back to Menu ──────────────┘
```

## Project structure

```
bank/
├── src/main.c      # Application source
├── assets/         # Optional music (not tracked)
├── Makefile        # Build script
└── bankapp.exe     # Original prebuilt Windows binary
```

## Quick start (Windows)

Run the prebuilt binary:

```powershell
.\bankapp.exe
```

> `bankapp.exe` is the original build. The source in `src/main.c` is a rebuild of the same app (same screens and flow), made after the original source was lost. When you build from source, the demo login PIN is **`1234`** and the starting balance is GHS 5,000.00.

## Building from source

1. Install raylib 5.x. On Windows, the [raylib installer](https://raysan5.itch.io/raylib) includes the `w64devkit` compiler.
2. Build:

   ```bash
   make RAYLIB_PATH=C:/raylib/raylib
   ```

   Or compile directly:

   ```bash
   gcc src/main.c -o bankapp.exe -std=c99 -O2 -IC:/raylib/raylib/src -LC:/raylib/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm
   ```

3. On Linux or macOS, with raylib installed system-wide: `make`.

## Customising the account

Edit the demo account at the top of `src/main.c`:

```c
static Account account = {
    .name          = "Prince Oscar",
    .accountNumber = "1002003004",
    .pin           = "1234",
    .balance       = 5000.00
};
```

> **Note:** this is a learning project. The data lives only in memory and resets every time the app starts.

---

Author: **Prince Oscar Mwinnuo** ([@Oscar006-HUB](https://github.com/Oscar006-HUB))
