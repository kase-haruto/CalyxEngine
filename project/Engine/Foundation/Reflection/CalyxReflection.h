#pragma once

// 反射生成ツールへオブジェクト型登録を通知するマーカー。
// C++上では空だが、Tools/Reflection/generate_reflection.* が読み取る。
#define CALYX_OBJECT(...)
#define CALYX_PLACEABLE_OBJECT(...)
#define CALYX_PROPERTY(...)
#define CALYX_GENERATED_BODY(...)
