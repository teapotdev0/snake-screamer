#Змейка, ползущая вверх по бесконечному лабиринту. Содержит скримеры.
##WARNING

**В игре есть скримеры и громкие звуки.** Если ты впечатлительный — не играй.

## Управление

| Клавиша | Действие |
|---|---|
| WASD / стрелки | движение |
| Enter / Space | старт |
| R | рестарт после смерти |
| M | в меню |
| Esc | выход |

## Установка (macOS)

```bash
brew install sdl2 sdl2_image sdl2_mixer
```

## Сборка

  bash
c++ -std=c++17 snake.cpp -o snake \
    -I/opt/homebrew/include \
    -I/opt/homebrew/include/SDL2 \
    -L/opt/homebrew/lib \
    -lSDL2 -lSDL2_image -lSDL2_mixer

## Запуск

 bash
./snake

## Зависимости

- SDL2
- SDL2_image
- SDL2_mixer

## Файлы

- snake.cpp 
- scream.png
- scream.wav
