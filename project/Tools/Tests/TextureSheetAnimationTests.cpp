#include <Engine/Objects/2D/Animation/SpriteAnimator2d.h>

#include <cassert>
#include <iostream>
#include <limits>

// Develop構成のエンジンDLLにリンクする。描画やアセットDBの初期化は不要。
// 対象未接続でも実際の再生処理を実行し、時間経過・終端・不正入力を検証する。
int main() {
    using namespace CalyxEngine;
    SpriteAnimator2d a;
    assert(a.SetTextureSheet("Textures/digits.png", 10, 1));
    a.ShowFrame(7);
    a.Update(10.0f);
    assert(a.GetCurrentFrame() == 7 && !a.IsPlaying());
    a.ShowFrame(-1);
    assert(a.GetCurrentFrame() == 0);
    a.ShowFrame(100);
    assert(a.GetCurrentFrame() == 9);

    assert(a.PlayFrames(2, 4, 0.125f, false));
    a.Update(0.375f);
    assert(a.GetCurrentFrame() == 5 && a.IsPlaying() && !a.IsFinished());
    a.Update(0.125f);
    assert(a.GetCurrentFrame() == 5 && !a.IsPlaying() && a.IsFinished());
    a.Update(100.0f);
    assert(a.GetCurrentFrame() == 5);

    a.SetReversed(true);
    assert(a.PlayFrames(2, 4, 0.125f, false));
    assert(a.GetCurrentFrame() == 5);
    a.Update(0.5f);
    assert(a.GetCurrentFrame() == 2 && a.IsFinished());
    assert(a.PlayFrames(2, 4, 0.125f, true));
    a.Update(0.625f);
    assert(a.GetCurrentFrame() == 4 && a.IsPlaying());

    a.SetReversed(false);
    assert(a.PlayFrames(2, 4, 0.125f, true));
    a.Update(1000000.25f);
    assert(a.GetCurrentFrame() == 4 && a.IsPlaying());
    a.Update(-1.0f);
    a.Update(std::numeric_limits<float>::infinity());
    a.Update(std::numeric_limits<float>::quiet_NaN());
    assert(a.GetCurrentFrame() == 4);
    assert(!a.PlayFrames(9, 2));
    assert(!a.PlayFrames(0, 0));
    assert(!a.PlayFrames(0, 1, 0));
    assert(!a.PlayFrames(0, 1, std::numeric_limits<float>::quiet_NaN()));
    assert(!a.SetTextureSheet("Textures/invalid.png", 0, 1));
    assert(!a.SetTextureSheet("Textures/invalid.png", 100000, 100000));
    assert(a.GetCurrentFrame() == 4 && a.IsPlaying());

    a.ShowFrame(8);
    assert(!a.IsPlaying() && !a.IsFinished());
    a.SetTexture("Textures/plain.png");
    assert(a.GetCurrentFrame() == 0 && !a.IsPlaying());
    assert(a.GetAnimationAsset()->GetFrameCapacity() == 1);

    auto shared = std::make_shared<SpriteAnimationAsset>();
    shared->division = {4, 2};
    shared->clips = {{"Hit", 4, 4, 0.125f, false}};
    a.SetAnimationAsset(shared);
    assert(a.Play("Hit"));
    a.Update(0.125f);
    assert(a.Play("Hit", false) && a.GetCurrentFrame() == 5);
    assert(!a.Play("Missing") && a.GetCurrentFrame() == 5);
    a.Reset();
    assert(a.GetCurrentFrame() == 4 && !a.IsPlaying());
    assert(a.PlayFrames(0, 2));
    assert(shared->clips.size() == 1 && shared->clips[0].name == "Hit");
    shared->division = {std::numeric_limits<float>::infinity(), -1};
    assert(shared->GetFrameCapacity() == 1);
    std::cout << "Texture sheet animation tests passed.\n";
}
