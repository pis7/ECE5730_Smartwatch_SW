#ifndef SSH1106_H
#define SSH1106_H 

#include "font_ssh1106.h"
#include "common.h"

/* SSH1106 settings */
/* SSH1106 width in pixels */
#ifndef SSH1106_WIDTH
#define SSH1106_WIDTH            132
#endif
/* SSH1106 LCD height in pixels */
#ifndef SSH1106_HEIGHT
#define SSH1106_HEIGHT           72
#endif

/**
 * @brief  SSH1106 color enumeration
 */
typedef enum {
	SSH1106_COLOR_BLACK = 0x00, /*!< Black color, no pixel */
	SSH1106_COLOR_WHITE = 0x01  /*!< Pixel is set. Color depends on LCD */
} SSH1106_COLOR_t;


/**
 * @brief  Initializes SSH1106 LCD
 * @param  None
 * @retval Initialization status:
 *           - 0: LCD was not detected on I2C port
 *           - > 0: LCD initialized OK and ready to use
 */
uint8_t SSH1106_Init(void);

/** 
 * @brief  Updates buffer from internal RAM to LCD
 * @note   This function must be called each time you do some changes to LCD, to update buffer from RAM to LCD
 * @param  None
 * @retval None
 */
void SSH1106_UpdateScreen(void);

/**
 * @brief  Toggles pixels invertion inside internal RAM
 * @note   @ref SSH1106_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  None
 * @retval None
 */
void SSH1106_ToggleInvert(void);

/** 
 * @brief  Fills entire LCD with desired color
 * @note   @ref SSH1106_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  Color: Color to be used for screen fill. This parameter can be a value of @ref SSH1106_COLOR_t enumeration
 * @retval None
 */
void SSH1106_Fill(SSH1106_COLOR_t Color);

/**
 * @brief  Draws pixel at desired location
 * @note   @ref SSH1106_UpdateScreen() must called after that in order to see updated LCD screen
 * @param  x: X location. This parameter can be a value between 0 and SSH1106_WIDTH - 1
 * @param  y: Y location. This parameter can be a value between 0 and SSH1106_HEIGHT - 1
 * @param  color: Color to be used for screen fill. This parameter can be a value of @ref SSH1106_COLOR_t enumeration
 * @retval None
 */
void SSH1106_DrawPixel(uint16_t x, uint16_t y, SSH1106_COLOR_t color);

/**
 * @brief  Sets cursor pointer to desired location for strings
 * @param  x: X location. This parameter can be a value between 0 and SSH1106_WIDTH - 1
 * @param  y: Y location. This parameter can be a value between 0 and SSH1106_HEIGHT - 1
 * @retval None
 */
void SSH1106_GotoXY(uint16_t x, uint16_t y);

/**
 * @brief  Puts character to internal RAM
 * @note   @ref SSH1106_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  ch: Character to be written
 * @param  *Font: Pointer to @ref FontDef_t structure with used font
 * @param  color: Color used for drawing. This parameter can be a value of @ref SSH1106_COLOR_t enumeration
 * @retval Character written
 */
char SSH1106_Putc(char ch, FontDef_t* Font, SSH1106_COLOR_t color);

/**
 * @brief  Puts string to internal RAM
 * @note   @ref SSH1106_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  *str: String to be written
 * @param  *Font: Pointer to @ref FontDef_t structure with used font
 * @param  color: Color used for drawing. This parameter can be a value of @ref SSH1106_COLOR_t enumeration
 * @retval Zero on success or character value when function failed
 */
char SSH1106_Puts(char* str, FontDef_t* Font, SSH1106_COLOR_t color);

/**
 * @brief  Draws line on LCD
 * @note   @ref SSH1106_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x0: Line X start point. Valid input is 0 to SSH1106_WIDTH - 1
 * @param  y0: Line Y start point. Valid input is 0 to SSH1106_HEIGHT - 1
 * @param  x1: Line X end point. Valid input is 0 to SSH1106_WIDTH - 1
 * @param  y1: Line Y end point. Valid input is 0 to SSH1106_HEIGHT - 1
 * @param  c: Color to be used. This parameter can be a value of @ref SSH1106_COLOR_t enumeration
 * @retval None
 */
void SSH1106_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, SSH1106_COLOR_t c);

/**
 * @brief  Draws rectangle on LCD
 * @note   @ref SSH1106_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x: Top left X start point. Valid input is 0 to SSH1106_WIDTH - 1
 * @param  y: Top left Y start point. Valid input is 0 to SSH1106_HEIGHT - 1
 * @param  w: Rectangle width in units of pixels
 * @param  h: Rectangle height in units of pixels
 * @param  c: Color to be used. This parameter can be a value of @ref SSH1106_COLOR_t enumeration
 * @retval None
 */
void SSH1106_DrawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, SSH1106_COLOR_t c);

/**
 * @brief  Draws filled rectangle on LCD
 * @note   @ref SSH1106_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x: Top left X start point. Valid input is 0 to SSH1106_WIDTH - 1
 * @param  y: Top left Y start point. Valid input is 0 to SSH1106_HEIGHT - 1
 * @param  w: Rectangle width in units of pixels
 * @param  h: Rectangle height in units of pixels
 * @param  c: Color to be used. This parameter can be a value of @ref SSH1106_COLOR_t enumeration
 * @retval None
 */
void SSH1106_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, SSH1106_COLOR_t c);

/**
 * @brief  Draws triangle on LCD
 * @note   @ref SSH1106_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x1: First coordinate X location. Valid input is 0 to SSH1106_WIDTH - 1
 * @param  y1: First coordinate Y location. Valid input is 0 to SSH1106_HEIGHT - 1
 * @param  x2: Second coordinate X location. Valid input is 0 to SSH1106_WIDTH - 1
 * @param  y2: Second coordinate Y location. Valid input is 0 to SSH1106_HEIGHT - 1
 * @param  x3: Third coordinate X location. Valid input is 0 to SSH1106_WIDTH - 1
 * @param  y3: Third coordinate Y location. Valid input is 0 to SSH1106_HEIGHT - 1
 * @param  c: Color to be used. This parameter can be a value of @ref SSH1106_COLOR_t enumeration
 * @retval None
 */
void SSH1106_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, SSH1106_COLOR_t color);

/**
 * @brief  Draws circle to STM buffer
 * @note   @ref SSH1106_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x: X location for center of circle. Valid input is 0 to SSH1106_WIDTH - 1
 * @param  y: Y location for center of circle. Valid input is 0 to SSH1106_HEIGHT - 1
 * @param  r: Circle radius in units of pixels
 * @param  c: Color to be used. This parameter can be a value of @ref SSH1106_COLOR_t enumeration
 * @retval None
 */
void SSH1106_DrawCircle(int16_t x0, int16_t y0, int16_t r, SSH1106_COLOR_t c);

/**
 * @brief  Draws filled circle to STM buffer
 * @note   @ref SSH1106_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x: X location for center of circle. Valid input is 0 to SSH1106_WIDTH - 1
 * @param  y: Y location for center of circle. Valid input is 0 to SSH1106_HEIGHT - 1
 * @param  r: Circle radius in units of pixels
 * @param  c: Color to be used. This parameter can be a value of @ref SSH1106_COLOR_t enumeration
 * @retval None
 */
void SSH1106_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, SSH1106_COLOR_t c);



#ifndef ssh1106_I2C_TIMEOUT
#define ssh1106_I2C_TIMEOUT					20000
#endif

/**
 * @brief  Initializes SSH1106 LCD
 * @param  None
 * @retval Initialization status:
 *           - 0: LCD was not detected on I2C port
 *           - > 0: LCD initialized OK and ready to use
 */
void ssh1106_I2C_Init();

/**
 * @brief  Writes single byte to slave
 * @param  *I2Cx: I2C used
 * @param  address: 7 bit slave address, left aligned, bits 7:1 are used, LSB bit is not used
 * @param  reg: register to write to
 * @param  data: data to be written
 * @retval None
 */
void ssh1106_I2C_Write(uint8_t address, uint8_t reg, uint8_t data);

/**
 * @brief  Writes multi bytes to slave
 * @param  *I2Cx: I2C used
 * @param  address: 7 bit slave address, left aligned, bits 7:1 are used, LSB bit is not used
 * @param  reg: register to write to
 * @param  *data: pointer to data array to write it to slave
 * @param  count: how many bytes will be written
 * @retval None
 */
void ssh1106_I2C_WriteMulti(uint8_t address, uint8_t reg, uint8_t *data, uint16_t count);

/**
 * @brief  Draws the Bitmap
 * @param  X:  X location to start the Drawing
 * @param  Y:  Y location to start the Drawing
 * @param  *bitmap : Pointer to the bitmap
 * @param  W : width of the image
 * @param  H : Height of the image
 * @param  color : 1-> white/blue, 0-> black
 */
void SSH1106_DrawBitmap(int16_t x, int16_t y, const unsigned char* bitmap, int16_t w, int16_t h, uint16_t color);

// scroll the screen for fixed rows

void SSH1106_ScrollRight(uint8_t start_row, uint8_t end_row);


void SSH1106_ScrollLeft(uint8_t start_row, uint8_t end_row);


void SSH1106_Scrolldiagright(uint8_t start_row, uint8_t end_row);


void SSH1106_Scrolldiagleft(uint8_t start_row, uint8_t end_row);



void SSH1106_Stopscroll(void);


// inverts the display i = 1->inverted, i = 0->normal

void SSH1106_InvertDisplay (int i);

// clear the display

void SSH1106_Clear (void);


#endif