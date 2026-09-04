# VocalChainOne — 9 плагинов в одном, под Mac Apple Silicon (arm64)

Цепочка обработки (порядок ФИКСИРОВАН, как просил пользователь):

1. **CorrectionEQ** — 90Hz -11dB / 1.5kHz +3.3dB / 8kHz +9.5dB *(точные цифры со скрина)*
2. **SimpleLimiter** — эмуляция Fruity Limiter (LIMIT-режим)
3. **VocalParametricEQ7** — 7-полосный EQ, реконструкция кривой
4. **VintageCompressor** — Threshold -7.4dB / Ratio 8:1 / Gain +15dB / Attack 60.7ms / Release 200ms *(точные цифры со скрина)*
5. **SoundgoodizerC** — эмуляция режима C
6. **FastDistStage** — режим A, **wet 20%** *(точный процент от пользователя)*
7. **FreshAirStage** — оба параметра база +8/10 *(точное значение от пользователя)*
8. **FlangusStage** — **wet 17%** *(точный процент от пользователя)*
9. **ReverbStage** — режим MID, **wet 6%** *(точный процент от пользователя)*

## 5 крутилок для пользователя

| Крутилка | Диапазон | Управляет |
|---|---|---|
| Теплее — Ярче | -50..+50 | CorrectionEQ + EQ7 наклон, Fresh Air баланс |
| Сжатие вокала | 0..100 | Threshold + Ratio компрессора |
| Громкость | 0..100 | Limiter (ceiling/gain) + Soundgoodizer amount |
| Грязь | 0..100 | Fast Dist Thresh/Post (wet фиксирован) |
| Пространство | 0..100 | Flangus + Reverb depth/size (wet фиксированы) |

## ⚠️ Что снято точно, а что приблизительно

**Точно (есть цифры на скринах):**
- CorrectionEQ (90/1.5k/8k)
- VintageCompressor (все 5 параметров)
- Fast Dist / Flangus / Reverb — **проценты wet**, которые ты назвал текстом

**Приблизительно (на скринах были только положения крутилок без цифр, восстановлено на глаз):**
- Fruity Limiter — Gain/Sat/Ceiling/Attack/Release/Sustain/Noise Gate
- Fruity Parametric EQ 2 — точные Hz/dB/Q всех 7 узлов
- Soundgoodizer — точная сила эффекта в режиме C
- Fast Dist — Pre/Thresh/Post (кроме wet)
- Fresh Air — точная формула эксайтера (база +8 сохранена)
- Flangus — Depth/Speed/Delay/Spread/Cross (кроме wet)
- Fruity Reverb 2 — Size/Decay/Diffusion/Damp (кроме wet)

**Это нормально и ожидаемо** — цифры без подписанных значений нельзя снять со скриншота точно. После первой сборки нужно будет свериться на слух с оригинальной цепочкой в FL Studio и сказать мне, где не хватает или перебор — я подкручу конкретные коэффициенты в коде.

## Сборка (arm64, НЕ Intel)

Тот же процесс, что и в прошлый раз — через GitHub Actions на облачном Mac:
```bash
cmake -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build --config Release
```
Готовый плагин: `build/VocalChainOne_artefacts/Release/VST3/VocalChainOne.vst3`

Флаг `-DCMAKE_OSX_ARCHITECTURES=arm64` жёстко пин\ит архитектуру именно под Apple Silicon — Intel-бинарник физически не соберётся. В GitHub Actions есть отдельный шаг "Verify architecture", который печатает результат `lipo -info` в лог сборки — там будет видно `arm64` и ничего больше.

## Дизайн

Текущий интерфейс — временная заглушка (5 крутилок без стилизации). Дизайн обсуждаем отдельно.
