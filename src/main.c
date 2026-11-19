/*
 * My Bank Application
 * -------------------
 * A simple ATM-style banking app with a graphical interface built on raylib.
 *
 * Features:
 *   - 4-digit PIN login
 *   - Account dashboard (user, account number, balance in GHS)
 *   - Withdraw, deposit and transfer with input validation
 *   - Confirmation screens after each transaction
 *   - Optional background music (assets/badbunny.mp3)
 *
 * Build: see README.md (requires raylib 5.x).
 */

#include "raylib.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Configuration                                                             */
/* ------------------------------------------------------------------------- */

#define SCREEN_WIDTH   800
#define SCREEN_HEIGHT  600
#define MUSIC_PATH     "assets/badbunny.mp3"

#define PIN_LENGTH     4
#define AMOUNT_MAX_LEN 10
#define ACCNUM_MAX_LEN 12

/* Palette */
#define COL_BG        (Color){ 18,  32,  47, 255 }
#define COL_PANEL     (Color){ 30,  50,  72, 255 }
#define COL_ACCENT    (Color){ 0,  168, 132, 255 }
#define COL_ACCENT_HI (Color){ 0,  200, 158, 255 }
#define COL_DANGER    (Color){ 214,  69,  65, 255 }
#define COL_DANGER_HI (Color){ 240,  90,  85, 255 }
#define COL_TEXT      (Color){ 236, 240, 241, 255 }
#define COL_MUTED     (Color){ 149, 165, 166, 255 }
#define COL_INPUT     (Color){ 44,  70,  98, 255 }

/* ------------------------------------------------------------------------- */
/* Types                                                                     */
/* ------------------------------------------------------------------------- */

typedef enum {
    SCREEN_LOGIN,
    SCREEN_DASHBOARD,
    SCREEN_WITHDRAW,
    SCREEN_DEPOSIT,
    SCREEN_TRANSFER,
    SCREEN_WITHDRAW_SUCCESS,
    SCREEN_DEPOSIT_SUCCESS,
    SCREEN_TRANSFER_SUCCESS
} Screen;

typedef struct {
    char   name[32];
    char   accountNumber[ACCNUM_MAX_LEN + 1];
    char   pin[PIN_LENGTH + 1];
    double balance;
} Account;

/* A single-line text box that accepts a restricted set of characters. */
typedef struct {
    Rectangle bounds;
    char      text[32];
    int       maxLen;
    bool      digitsOnly;   /* true: 0-9 only; false: also allows one '.' */
    bool      masked;       /* draw as '*' (for PINs) */
    bool      active;
} TextBox;

/* ------------------------------------------------------------------------- */
/* Application state                                                         */
/* ------------------------------------------------------------------------- */

/* Demo account. Change these values to customise the app. */
static Account account = {
    .name          = "Prince Oscar",
    .accountNumber = "1002003004",
    .pin           = "1234",
    .balance       = 5000.00
};

static Screen currentScreen = SCREEN_LOGIN;
static char   errorMsg[64]  = "";

static TextBox pinBox;
static TextBox amountBox;
static TextBox receiverBox;

/* Result of the last transaction, shown on the success screens. */
static double lastAmount = 0.0;
static char   lastReceiver[ACCNUM_MAX_LEN + 1] = "";

/* ------------------------------------------------------------------------- */
/* UI helpers                                                                */
/* ------------------------------------------------------------------------- */

static void DrawCenteredText(const char *text, int y, int size, Color color)
{
    int w = MeasureText(text, size);
    DrawText(text, (SCREEN_WIDTH - w) / 2, y, size, color);
}

/* Draws a button and returns true when it is clicked. */
static bool Button(Rectangle r, const char *label, Color base, Color hover)
{
    bool over = CheckCollisionPointRec(GetMousePosition(), r);
    DrawRectangleRounded(r, 0.25f, 8, over ? hover : base);
    int size = 22;
    int w = MeasureText(label, size);
    DrawText(label, (int)(r.x + (r.width - w) / 2), (int)(r.y + (r.height - size) / 2), size, COL_TEXT);
    return over && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

static TextBox MakeTextBox(Rectangle bounds, int maxLen, bool digitsOnly, bool masked)
{
    TextBox tb = { 0 };
    tb.bounds     = bounds;
    tb.maxLen     = maxLen;
    tb.digitsOnly = digitsOnly;
    tb.masked     = masked;
    return tb;
}

static void ClearTextBox(TextBox *tb)
{
    tb->text[0] = '\0';
    tb->active  = false;
}

/* Handles focus and keyboard input for a text box. */
static void UpdateTextBox(TextBox *tb)
{
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        tb->active = CheckCollisionPointRec(GetMousePosition(), tb->bounds);

    if (!tb->active) return;

    int len = (int)strlen(tb->text);
    int ch  = GetCharPressed();
    while (ch > 0) {
        bool ok = (ch >= '0' && ch <= '9');
        if (!tb->digitsOnly && ch == '.' && strchr(tb->text, '.') == NULL) ok = true;
        if (ok && len < tb->maxLen) {
            tb->text[len++] = (char)ch;
            tb->text[len]   = '\0';
        }
        ch = GetCharPressed();
    }

    if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && len > 0)
        tb->text[len - 1] = '\0';
}

static void DrawTextBox(const TextBox *tb)
{
    DrawRectangleRounded(tb->bounds, 0.2f, 8, COL_INPUT);
    DrawRectangleLinesEx(tb->bounds, 2.0f, tb->active ? COL_ACCENT : COL_MUTED);

    char shown[32];
    if (tb->masked) {
        int n = (int)strlen(tb->text);
        for (int i = 0; i < n; i++) shown[i] = '*';
        shown[n] = '\0';
    } else {
        strcpy(shown, tb->text);
    }

    int size = 26;
    int tx = (int)tb->bounds.x + 14;
    int ty = (int)(tb->bounds.y + (tb->bounds.height - size) / 2);
    DrawText(shown, tx, ty, size, COL_TEXT);

    /* Blinking caret */
    if (tb->active && ((int)(GetTime() * 2) % 2 == 0))
        DrawText("_", tx + MeasureText(shown, size) + 2, ty, size, COL_ACCENT);
}

static void DrawHeader(const char *title)
{
    DrawRectangle(0, 0, SCREEN_WIDTH, 80, COL_PANEL);
    DrawCenteredText(title, 25, 32, COL_TEXT);
}

static void DrawError(int y)
{
    if (errorMsg[0] != '\0') DrawCenteredText(errorMsg, y, 20, COL_DANGER);
}

static void GoTo(Screen s)
{
    currentScreen = s;
    errorMsg[0]   = '\0';
    ClearTextBox(&amountBox);
    ClearTextBox(&receiverBox);
}

/* Parses the amount box. Returns false (and sets errorMsg) if invalid. */
static bool ReadAmount(double *out)
{
    if (amountBox.text[0] == '\0') {
        strcpy(errorMsg, "Please enter an amount.");
        return false;
    }
    double v = atof(amountBox.text);
    if (v <= 0.0) {
        strcpy(errorMsg, "Amount must be greater than zero.");
        return false;
    }
    *out = v;
    return true;
}

/* ------------------------------------------------------------------------- */
/* Screens                                                                   */
/* ------------------------------------------------------------------------- */

static void ScreenLogin(void)
{
    UpdateTextBox(&pinBox);

    DrawCenteredText("WELCOME TO THE BANK", 120, 40, COL_ACCENT);
    DrawCenteredText("Enter your 4-digit PIN:", 220, 24, COL_TEXT);
    DrawTextBox(&pinBox);

    Rectangle loginBtn = { SCREEN_WIDTH / 2.0f - 100, 380, 200, 55 };
    bool submit = Button(loginBtn, "Login", COL_ACCENT, COL_ACCENT_HI) || IsKeyPressed(KEY_ENTER);

    if (submit) {
        if (strcmp(pinBox.text, account.pin) == 0) {
            TraceLog(LOG_INFO, "User logged in successfully");
            ClearTextBox(&pinBox);
            GoTo(SCREEN_DASHBOARD);
        } else {
            TraceLog(LOG_WARNING, "Failed login attempt");
            strcpy(errorMsg, "Incorrect PIN!");
            ClearTextBox(&pinBox);
            pinBox.active = true;
        }
    }

    DrawError(460);
}

static void ScreenDashboard(void)
{
    DrawHeader("ACCOUNT DASHBOARD");

    DrawRectangleRounded((Rectangle){ 150, 110, 500, 140 }, 0.15f, 8, COL_PANEL);
    DrawText(TextFormat("User: %s", account.name),          180, 130, 24, COL_TEXT);
    DrawText(TextFormat("Acc #: %s", account.accountNumber), 180, 165, 24, COL_MUTED);
    DrawText(TextFormat("Balance: GHS %.2f", account.balance), 180, 205, 28, COL_ACCENT);

    float bw = 240, bh = 60, gap = 20;
    float x1 = SCREEN_WIDTH / 2.0f - bw - gap / 2, x2 = SCREEN_WIDTH / 2.0f + gap / 2;

    if (Button((Rectangle){ x1, 290, bw, bh }, "Withdraw", COL_ACCENT, COL_ACCENT_HI)) GoTo(SCREEN_WITHDRAW);
    if (Button((Rectangle){ x2, 290, bw, bh }, "Deposit",  COL_ACCENT, COL_ACCENT_HI)) GoTo(SCREEN_DEPOSIT);
    if (Button((Rectangle){ x1, 370, bw, bh }, "Transfer", COL_ACCENT, COL_ACCENT_HI)) GoTo(SCREEN_TRANSFER);
    if (Button((Rectangle){ x2, 370, bw, bh }, "Logout",   COL_DANGER, COL_DANGER_HI)) {
        GoTo(SCREEN_LOGIN);
        pinBox.active = true;
    }
}

static void ScreenWithdraw(void)
{
    UpdateTextBox(&amountBox);
    DrawHeader("WITHDRAW FUNDS");

    DrawText(TextFormat("Balance: GHS %.2f", account.balance), 200, 120, 24, COL_MUTED);
    DrawText("Enter Amount to Withdraw:", 200, 180, 24, COL_TEXT);
    DrawTextBox(&amountBox);

    if (Button((Rectangle){ 200, 330, 190, 55 }, "Confirm", COL_ACCENT, COL_ACCENT_HI) || IsKeyPressed(KEY_ENTER)) {
        double amt;
        if (ReadAmount(&amt)) {
            if (amt > account.balance) {
                strcpy(errorMsg, "Insufficient funds.");
            } else {
                account.balance -= amt;
                lastAmount = amt;
                GoTo(SCREEN_WITHDRAW_SUCCESS);
                return;
            }
        }
    }
    if (Button((Rectangle){ 410, 330, 190, 55 }, "Cancel", COL_DANGER, COL_DANGER_HI)) { GoTo(SCREEN_DASHBOARD); return; }

    DrawError(410);
}

static void ScreenDeposit(void)
{
    UpdateTextBox(&amountBox);
    DrawHeader("DEPOSIT FUNDS");

    DrawText(TextFormat("Balance: GHS %.2f", account.balance), 200, 120, 24, COL_MUTED);
    DrawText("Enter Amount to Deposit:", 200, 180, 24, COL_TEXT);
    DrawTextBox(&amountBox);

    if (Button((Rectangle){ 200, 330, 190, 55 }, "Confirm", COL_ACCENT, COL_ACCENT_HI) || IsKeyPressed(KEY_ENTER)) {
        double amt;
        if (ReadAmount(&amt)) {
            account.balance += amt;
            lastAmount = amt;
            GoTo(SCREEN_DEPOSIT_SUCCESS);
            return;
        }
    }
    if (Button((Rectangle){ 410, 330, 190, 55 }, "Cancel", COL_DANGER, COL_DANGER_HI)) { GoTo(SCREEN_DASHBOARD); return; }

    DrawError(410);
}

static void ScreenTransfer(void)
{
    UpdateTextBox(&receiverBox);
    UpdateTextBox(&amountBox);
    DrawHeader("TRANSFER FUNDS");

    DrawText(TextFormat("From Account: %s", account.accountNumber), 200, 105, 22, COL_MUTED);
    DrawText(TextFormat("Balance: GHS %.2f", account.balance),       200, 135, 22, COL_MUTED);

    DrawText("Receiver Account #:", 200, 180, 22, COL_TEXT);
    DrawTextBox(&receiverBox);
    DrawText("Amount to Send:", 200, 290, 22, COL_TEXT);
    DrawTextBox(&amountBox);

    if (Button((Rectangle){ 200, 420, 190, 55 }, "Send", COL_ACCENT, COL_ACCENT_HI)) {
        double amt;
        if (strlen(receiverBox.text) < 6) {
            strcpy(errorMsg, "Enter a valid receiver account number.");
        } else if (strcmp(receiverBox.text, account.accountNumber) == 0) {
            strcpy(errorMsg, "Cannot transfer to your own account.");
        } else if (ReadAmount(&amt)) {
            if (amt > account.balance) {
                strcpy(errorMsg, "Insufficient funds.");
            } else {
                account.balance -= amt;
                lastAmount = amt;
                strcpy(lastReceiver, receiverBox.text);
                GoTo(SCREEN_TRANSFER_SUCCESS);
                return;
            }
        }
    }
    if (Button((Rectangle){ 410, 420, 190, 55 }, "Cancel", COL_DANGER, COL_DANGER_HI)) { GoTo(SCREEN_DASHBOARD); return; }

    DrawError(500);
}

/* Shared layout for the three success screens. */
static void ScreenSuccess(const char *title, const char *line1, const char *line2)
{
    DrawCenteredText(title, 140, 36, COL_ACCENT);
    DrawRectangleRounded((Rectangle){ 175, 210, 450, 150 }, 0.15f, 8, COL_PANEL);
    DrawCenteredText(line1, 240, 24, COL_TEXT);
    if (line2) DrawCenteredText(line2, 280, 24, COL_TEXT);
    DrawCenteredText(TextFormat("New Balance: GHS %.2f", account.balance), line2 ? 320 : 290, 24, COL_ACCENT);

    if (Button((Rectangle){ SCREEN_WIDTH / 2.0f - 120, 410, 240, 55 }, "Back to Menu", COL_ACCENT, COL_ACCENT_HI)
        || IsKeyPressed(KEY_ENTER))
        GoTo(SCREEN_DASHBOARD);
}

/* ------------------------------------------------------------------------- */
/* Entry point                                                               */
/* ------------------------------------------------------------------------- */

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "My Bank Application");
    SetTargetFPS(60);

    /* Background music is optional: the app runs fine without the file. */
    InitAudioDevice();
    Music music = { 0 };
    bool hasMusic = false;
    if (FileExists(MUSIC_PATH)) {
        music = LoadMusicStream(MUSIC_PATH);
        PlayMusicStream(music);
        hasMusic = true;
    }

    pinBox      = MakeTextBox((Rectangle){ SCREEN_WIDTH / 2.0f - 100, 270, 200, 60 }, PIN_LENGTH,     true,  true);
    amountBox   = MakeTextBox((Rectangle){ 200, 220, 400, 55 },                      AMOUNT_MAX_LEN, false, false);
    receiverBox = MakeTextBox((Rectangle){ 200, 215, 400, 55 },                      ACCNUM_MAX_LEN, true,  false);
    pinBox.active = true;

    while (!WindowShouldClose()) {
        if (hasMusic) UpdateMusicStream(music);

        /* The transfer screen stacks two inputs, so the amount box moves down there. */
        amountBox.bounds.y = (currentScreen == SCREEN_TRANSFER) ? 325 : 220;

        BeginDrawing();
        ClearBackground(COL_BG);

        switch (currentScreen) {
            case SCREEN_LOGIN:     ScreenLogin();     break;
            case SCREEN_DASHBOARD: ScreenDashboard(); break;
            case SCREEN_WITHDRAW:  ScreenWithdraw();  break;
            case SCREEN_DEPOSIT:   ScreenDeposit();   break;
            case SCREEN_TRANSFER:  ScreenTransfer();  break;
            case SCREEN_WITHDRAW_SUCCESS:
                ScreenSuccess("WITHDRAWAL SUCCESSFUL",
                              TextFormat("Amount Withdrawn: GHS %.2f", lastAmount), NULL);
                break;
            case SCREEN_DEPOSIT_SUCCESS:
                ScreenSuccess("DEPOSIT SUCCESSFUL",
                              TextFormat("Amount Deposited: GHS %.2f", lastAmount), NULL);
                break;
            case SCREEN_TRANSFER_SUCCESS: {
                char sentTo[48];
                snprintf(sentTo, sizeof sentTo, "Sent To: %s", lastReceiver);
                ScreenSuccess("TRANSFER SUCCESSFUL", sentTo,
                              TextFormat("Amount Sent: GHS %.2f", lastAmount));
            } break;
        }

        EndDrawing();
    }

    if (hasMusic) UnloadMusicStream(music);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
