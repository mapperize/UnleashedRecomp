#include "tas_windows.h"
#include <SWA.inl>

PPCContext savedRetryCtx;
uint8_t *savedRetryBase{};

PPCContext saved3DCtx;
uint8_t *saved3DBase{};

PPCContext saved2DCtx;
uint8_t *saved2DBase{};

#define PRINT_HOOK(func) \
PPC_FUNC_IMPL(__imp__##func);\
PPC_FUNC(func)\
{\
    __imp__##func(ctx, base);\
    printf("\n%s", #func);\
}

#define PRINT_ARGUMENTS(func) \
PPC_FUNC_IMPL(__imp__##func);\
PPC_FUNC(func)\
{\
    printf("\nr3: %x, r4: %x\n", ctx.r3.u32, ctx.r4.u32);\
    printf(" %s", #func);\
    __imp__##func(ctx, base);\
}

#define PRINT_RETURN(func) \
PPC_FUNC_IMPL(__imp__##func);\
PPC_FUNC(func)\
{\
    __imp__##func(ctx, base);\
    printf("\n%s", #func);\
    printf(" %x", ctx.r3.u32);\
}

#define PRINT_BOTH(func) \
PPC_FUNC_IMPL(__imp__##func);\
PPC_FUNC(func)\
{\
    printf("\nr3: %x, r4: %x, r5: %x", ctx.r3.u32, ctx.r4.u32, ctx.r5.u32);\
    printf(" %s", #func);\
    __imp__##func(ctx, base);\
    printf(" %x", ctx.r3.u32);\
}

void GetRotate(PPCRegister& r3){
    if (TASWindow::getDayTimeRotation) {
        TASWindow::rotation = (Quaternion*)g_memory.Translate(r3.u32 + 0x460);
    }
}

// sub_823176A0 insta kills day sonic to void
PPC_FUNC_IMPL(__imp__sub_823176A0);
PPC_FUNC(sub_823176A0){
    if (TASWindow::disableVoidKill)
        return;
    __imp__sub_823176A0(ctx, base);
}

// reloads game objects but crashes sometimes because it doesnt unload it?
// seems to not crash on night sonic for now
PPC_FUNC_IMPL(__imp__sub_827B62E0);
PPC_FUNC(sub_827B62E0){
    savedRetryCtx = ctx;
    savedRetryBase = base;
    __imp__sub_827B62E0(ctx, base);
}

// restarts the game to a state like death
/*PPC_FUNC_IMPL(__imp__sub_82304270);
PPC_FUNC(sub_82304270){
    savedRetryCtx = ctx;
    savedRetryBase = base;
    __imp__sub_82304270(ctx, base);
}*/

// checkpoints activate this to change the restart to the checkpoint 
PPC_FUNC_IMPL(__imp__sub_82305DF8);
PPC_FUNC(sub_82305DF8){
    if (TASWindow::isCheckpointDisable) return;
    __imp__sub_82305DF8(ctx, base);
}

// check if player is in 2d
PPC_FUNC_IMPL(__imp__sub_823538E0);
PPC_FUNC(sub_823538E0){
    __imp__sub_823538E0(ctx, base);
    TASWindow::is2DHook = ctx.r3.u32;
}

// the two functions below have weird parameters 
// the first parameter changes based on something being malloced when it's within a certain distance (i presume from culling)
// the second parameter has an id unique to each stage

// changes mode to 2d
PPC_FUNC_IMPL(__imp__sub_825F5B90);
PPC_FUNC(sub_825F5B90)
{
    //printf("\nr3: %x, r4: %d\n", ctx.r3.u32, ctx.r4.u32);
    saved2DCtx = ctx;
    saved2DBase = base;
    __imp__sub_825F5B90(ctx, base);
}
// changes mode to 3d
PPC_FUNC_IMPL(__imp__sub_825F5E40);
PPC_FUNC(sub_825F5E40)
{
    //printf("\nr3: %x, r4: %d\n", ctx.r3.u32, ctx.r4.u32);
    saved3DCtx = ctx;
    saved3DBase = base;
    __imp__sub_825F5E40(ctx, base);
}

// updates life counter after death
PPC_FUNC_IMPL(__imp__sub_8251A728);
PPC_FUNC(sub_8251A728)
{
    if (TASWindow::disableLives)
        return;
    __imp__sub_825F5E40(ctx, base);
}


/*

void TestHook(PPCRegister& r11){
    //printf("\n%x\n", r11.u32);
}

*/