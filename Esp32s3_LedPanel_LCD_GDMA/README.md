# ESP32S3_LED_PANEL_HUB75E_ICN2037_LCD_DMA
- This project uses the ESP32-S3 to drive a 128x64 LED panel powered by the ICN2037 LED driver.
- Display data is read from an SD card and buffered in memory before being streamed to the LED panel using DMA through the ESP32-S3 LCD_CAM peripheral. This approach allows efficient high-speed data transfer with minimal CPU overhead.
- The project is structured using modular components, making it easier to maintain and extend.

## Table of Contents
- Features
- Demo
- System Architecture
- Memory Calculation
- Hardware Requirements
- Image Transmission Timing
- Image Raw Queue
- Gamma Correction
- Limitations
- Future Improvements

## Features
- Low CPU usage: The CPU is mainly used for calculation and data preparation, while data transfer to the LED panel is handled by DMA through the ESP32-S3 LCD_CAM peripheral.
- 8-bit color control: Supports up to **2^24 colors** for rich and detailed LED panel display.
- Gamma correction LUT: Uses a gamma correction lookup table to improve color accuracy and visual quality on the LED panel.
- Raw image queue: Implements a raw image queue to reduce RAM usage when loading image data.
- Render queue system: A dedicated render queue is used to maximize FPS when images change, allowing smooth updates on the display.
- SD card storage: Uses an SD card as external storage, enabling large amounts of image data and easy content expansion.

## Demo
https://github.com/user-attachments/assets/479dea61-e4d8-4dc6-bb64-c40b707f2991

## System Architecture
- SD Card → PSRAM Queue Image Raw → Render Queue → GDMA Descriptor → LCD_CAM → LED Panel

## Hardware Requirements
- ESP32S3 N16R8 (Or ESP32-S3 with PSRAM) 
- LedPanel with ICN2037(See the details below for supported LED panel configurations.)
- SD Card Spi Module

### ESP32S3 N16R8
- An SPI peripheral is required to interface with the SD card SPI module.
<p align="center">
    <img src="include/image/Esp32 s3 N16R8.png" width="400"><br>
    <em>Fig 1. ESP32S3 N16R8</em>
</p>

### Led Panel with ICN2037
- ICN 2037 datasheet: https://www.eagerled.com/wp-content/uploads/2024/04/ICN2037_datasheet_EN_2017_V2.0.pdf

<p align="center">
    <img src="include/image/Led Pannel.jpg" width="400"><br>
    <em>Fig 2. Led Panel</em>
</p>

- Using the HUB75E interface, specific rows of the LED panel can be selected for display.
- Based on the datasheet, the ICN2037 is controlled by the CLK, LATCH, and OE signals.

### SD Card Spi Module
- Data is exchanged with the ESP32-S3 via SPI communication.
<p align="center">
    <img src="include/image/SdCardSoiModule.jfif" width="400"><br>
    <em>Fig 3. SD Card Spi Module</em>
</p>

## Memory Calculation
### DMA Descriptor Memory Calculation:
- The number of DMA descriptors required is:
		height / 2 × n (color bitplanes) + 1(DUMMY PHASE)
		
- Each DMA descriptor requires one buffer, and each buffer must contain:
		2 bytes × (width + 2)
		
- Therefore, the total buffer memory required (excluding DMA descriptor structures) can be calculated as:
		Memory = (height / 2 × (width + 2) × n + 1) × 2 bytes
		
- Note: The reason for this formula will be explained in the following section.

### Display Timing Recommendation
- The project is optimized for **8-bit color depth**.

- When using 8-bit color representation, the total display time requires **255T**, where the bitplane timing follows binary weighting:
	1 bit  = 1T  
	2 bit  = 2T  
	3 bit  = 4T  
	...  
	8 bit  = 128T  

- Total time per frame:
	1T + 2T + 4T + ... + 128T = 255T

- To achieve stable refresh rates with this timing configuration, the recommended panel configuration is:
	+ Width ≥ 128 pixels
	+ Scan rate: 1/32

- These settings help maintain a good refresh rate and reduce visible flickering on the LED panel.
- Finally, based on the ICN2037 driver and the ESP32-S3 LCD_CAM peripheral, the display operation is divided into two main phases:
	+ OE phase
	+ CLK phase (which already includes the **LATCH phase**)
- In addition, due to the current implementation design, one extra **dummy phase** is inserted.

- Therefore, the total number of clock cycles required for one row illumination can be expressed as:
	Row cycles = (width + 2)
	
### Data Bus Width
- Based on the LED panel hardware design, 14 control pins are required.  
- Therefore, a **16-bit data mode (2 bytes) is used to control all signals.

### Row Scanning Mechanism
- The LED panel operates using a **row scanning mechanism**.  
- The panel used in this project has a **1/32 scan rate**.

- Because the panel drives the **upper and lower halves simultaneously**, the number of rows that need to be scanned is:
	+ Rows per scan cycle = height / 2

- A full scan of all rows corresponds to **one bitplane display cycle**.  
- Therefore, to display **n-bit color**, the total number of scan operations required is:
	+ Total scans = (height / 2) × n

### Memory Allocation Limitation
- To avoid memory fragmentation, the DMA buffers are allocated as a **single contiguous memory block** using the size calculated in the previous section.
- For example:
	+ Panel configuration: Width: 128, height: 64, scan rate: 1/32, color depth: 8-bit
	+ The required buffer memory is approximately **67 KB**.
- However, when increasing the panel width, the memory requirement grows quickly.  
- For instance, if the width is increased to **256 pixels**, the required memory becomes approximately **155 KB**.
- The major limitation is that the **largest contiguous memory block available is around 110 KB**, which means such configurations may fail to allocate the required buffer.
- Therefore, this memory limitation must be considered carefully when increasing the panel resolution.

## Image Transmission Timing (n-bit Color)
- As explained in the previous sections, the panel uses **bitplane PWM** to display colors.
- To ensure stable data transmission to the **ICN2037 driver**, a safe clock timing must be maintained.
- The safe clock period is **50 ns**, which ensures that data can propagate correctly from **OE LOW to OUT1**.
- In this implementation, data is updated on the **falling edge of CLK**, while the **ICN2037 latches data on the rising edge**.
- Therefore, to avoid data corruption, the effective clock period must be:
	Tclk = 50 ns × 2 = **100 ns**

- It is also important to note that:
	+ The **ABCDE row selection pins** directly control the row address and **do not pass through the ICN2037**, so their propagation delay is minimal.
	+ All other control signals pass through the **ICN2037 driver**, which introduces additional propagation delay.

- Combining this with the previous timing analysis, the total time required to transmit one image frame with **n-bit color** can be estimated as:
	Frame time = Tclk × (width + 2) × (height / 2) × n
- For this project (8-bit color):
	Frame time = **Tclk × (width + 2) × (height / 2) × 8**

## Render Queue
- As mentioned earlier, the **Render Queue** is designed to maximize the achievable FPS when a new image needs to be displayed.
- The mechanism is similar to a standard queue. However, there is an important constraint related to **DMA operation**. When DMA transfers data to the **ESP32-S3 LCD_CAM peripheral**, it continuously reads memory from the **DMA descriptor list and its associated buffers**.
- This means that **modifying the memory currently used by DMA is unsafe**, since the DMA controller may read partially updated data. The ESP-IDF documentation also does **not recommend modifying DMA buffers while a transfer is in progress**.
- To solve this issue, the system uses a **separate DMA descriptor region for rendering**:
	1. While the current frame is being displayed, a **new DMA descriptor set** is created in another memory region.
	2. The next image is processed and written into the buffers associated with these new descriptors.
	3. When the frame is ready to be displayed, the LCD transmission is **temporarily stopped**.
	4. The system then **switches the DMA descriptor pointer** to the new descriptor region.
	5. Finally, the LCD transmission is restarted and the new image is displayed.

This approach ensures that **DMA never reads from memory that is being modified**, while also allowing the next frame to be prepared in parallel.

## Raw Image Queue
- To simplify the rendering process and reduce system overhead, the project uses **raw image data** for display.
- In many implementations, RGB pixels are stored in a **uint32_t** (or `unsigned long`), which requires **4 bytes per pixel**.  
- For example:
	128 × 64 × 4 bytes = **32768 bytes**

- In this project, the image data is stored using **uint8_t values** for each RGB channel instead.  
- This means each pixel uses **3 bytes (R, G, B)** rather than 4 bytes.
- Therefore, the memory requirement becomes:
	128 × 64 × 3 bytes = **24576 bytes**
	
- This reduces the memory usage compared to the common 32-bit RGB representation.
- The **Raw Image Queue** functions like a standard queue whose purpose is to store raw image frames.  
- Each image buffer is allocated in **PSRAM** to minimize the usage of internal SRAM, which is reserved for critical components such as DMA buffers and descriptors.

## Color Balance and Gamma Correction

The brightness perceived by the human eye and the actual brightness emitted by LED panels are both non-linear. Therefore, raw RGB values must be adjusted before displaying.

### Color Balance
- Different LED colors have different brightness efficiencies, so each channel is scaled:

	R' = R × Kr  
	G' = G × Kg  
	B' = B × Kb  

- Where `Kr`, `Kg`, `Kb` are balance coefficients for Red, Green, and Blue.

### Gamma Correction
- To compensate for the non-linear perception of brightness, gamma correction is applied:
	Vout = 255 × (Vin / 255)^γ

- Where:
	`Vin` is the input value (0–255)
	`γ` is typically between **2.0 – 2.4**

- For performance, the system uses a **Gamma Lookup Table (LUT)**:
	LUT[i] = 255 × (i / 255)^γ

- This avoids expensive runtime calculations while improving color accuracy on the LED panel.
- Depending on the panel configuration, the system may require a lower color depth (e.g., 6-bit instead of 8-bit).  
- Therefore, RGB values are converted to match the configured bit depth before generating the display buffer.

## SD Card Driver
- A customized SPI SD card driver is used in this project to read image data from storage.
- This part mainly focuses on the driver implementation and data structures that are tailored to fit the needs of this project. Since the implementation details are not directly related to the LED panel rendering pipeline, they will not be discussed in detail here.

## Limitation
- During the latch operation, the clock signal is triggered simultaneously.  
- This behavior does not fully match the timing requirements specified in the ICN2037 datasheet.
- As a result, when displaying a colored pixel surrounded by black pixels, unintended color leakage may occur.  
- Some neighboring pixels (typically one of the four adjacent pixels) may briefly show the same color due to timing inaccuracies.

### Future Improvements
- First, the current project is implemented using bare-metal programming, which does not fully utilize the capabilities of the ESP32-S3. In the next stage, the system will be migrated to **FreeRTOS** to better leverage the dual-core architecture of the ESP32-S3. Planned architecture:
	+ **CPU0**: Handle WiFi communication.
	+ **CPU1**: Process image update tasks and manage SD card operations.
	+ **WiFi Interface**: Allow users to upload or delete raw image files stored on the SD card.
	+ **Control Application**: A companion app will be developed to control and manage the system.

- Finally, reduce signal noise and timing artifacts to ensure colors are displayed with full accuracy.
