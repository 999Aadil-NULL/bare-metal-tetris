# ARM-Tetris-BareMetal

A high-performance Tetris clone written in **Bare-Metal C** for the **ARM Cortex-A9** processor. This project bypasses any Operating System, communicating directly with FPGA hardware via memory-mapped I/O to handle real-time VGA rendering, interrupt-like input polling, and digital audio synthesis.

## 🕹️ Game Architecture

The game is built on a "legacy-first" architecture, drawing inspiration from early 8-bit consoles.

* **Matrix System:** Each Tetromino is represented as a $4 \times 4$ matrix. Collision detection and rotation are calculated by performing boundary checks against a $10 \times 20$ `board` array before rendering.
* **Memory-Mapped I/O (MMIO):** The engine uses specific memory addresses to control hardware peripherals:
    * **VGA Pixel Buffer (`0xC8000000`):** Direct pixel manipulation for the $320 \times 240$ display.
    * **Character Buffer (`0xC9000000`):** Used for rendering text overlays like "GAME OVER".
    * **PS/2 Controller (`0xFF200100`):** Decodes raw make/break scan codes from a keyboard.
    * **Pushbuttons (`0xFF200050`):** Uses edge-capture registers to handle "falling edge" button presses for precise movement.
* **The Game Loop:** A synchronous loop that utilizes a `smart_delay()` function. This ensures the game logic (falling pieces) stays decoupled from the audio synthesis, preventing the music from stuttering when the game pauses or resets.

---

## 🔊 8-Bit Audio Synthesis

The audio engine features a custom-built **Square Wave Synthesizer** that plays the classic 8-bit Tetris theme (Korobeiniki). 

### How it Works:
1.  **Digital Signal Generation:** Instead of playing a pre-recorded file, the code generates a square wave in real-time. It toggles the audio output between a high and low volume constant:
    $$Output = \begin{cases} +Volume & \text{if } Phase < 50\% \\ -Volume & \text{if } Phase \ge 50\% \end{cases}$$
2.  **Phase Stepping:** To achieve the correct musical pitch, the `wave_phase` increments based on the target frequency ($f$) and the hardware sample rate ($48\text{ kHz}$):
    $$\text{Phase Step} = \frac{f \times 100,000}{\text{Sample Rate}}$$
3.  **Hardware FIFO Buffers:** The code monitors the audio controller's FIFO (First-In, First-Out) buffers at `0xFF203040`. It only writes new samples when space is available, ensuring a continuous, glitch-free 48kHz audio stream.
4.  **Orchestration:** Notes and durations are stored in arrays (`tetris_freqs` and `tetris_durs`). A `sample_counter` tracks the progression of each note to handle timing and rests between pulses.

---

## 🚀 Key Features

* **Bare-Metal Performance:** Zero OS overhead; the code runs directly on the CPU for maximum responsiveness.
* **Dual-Input System:** Supports both external PS/2 keyboards and on-board FPGA pushbuttons.
* **Visual Feedback:** Clear-line animations and a "Game Over" state rendered via the VGA character buffer.
* **Responsive Physics:** Implements "soft drop" (speeding up the fall when holding the Down key) and rotation wall-kicking checks.

## 🛠️ Hardware Requirements

* **Target Board:** DE1-SoC (or any Altera/Intel SoC FPGA with an ARM Cortex-A9).
* **Simulator:** Fully compatible with [CPULator](https://cpulator.01xz.net/).
* **Display:** VGA monitor (640x480 or 320x240 compatible).

## ⌨️ Controls

| Action | Keyboard (PS/2) | Hardware Buttons |
| :--- | :--- | :--- |
| **Move Left/Right** | Left / Right Arrows | KEY 0 / KEY 1 |
| **Rotate Piece** | Up Arrow | KEY 2 |
| **Soft Drop** | Hold Down Arrow | N/A |
| **Restart** | Click Pushbutton 3 | KEY 3 |

---

## 📄 License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
