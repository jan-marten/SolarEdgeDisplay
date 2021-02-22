#include <U8g2lib.h>

// Class for using the 128x64 OLED display 
class display
{
    private:
        U8G2 _u8g2;

        // Display-Height: 16 pixels geel, 48 pixels blauw
        // Display-Width: 128 pixels
        
        const int marginLeft = 8; // space for tiny labels on the left
        const int marginTop = 16; // top is yellow-header, rest is blue for charting
        const int marginBottom = 8; // space for tiny labels at the bottom
        const int width = 128 - marginLeft;
        const int height =  64 - marginTop - marginBottom; // onderaan ruimte voor labels

    public:

        void Init(void)
        {
            // De complete lijst voor alle schermen vind je op: https://github.com/olikraus/u8g2/wiki/u8g2setupcpp
             U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
            // U8G2_SSD1306_128X64_NONAME_1_HW_I2C _u8g2; //1=uC-ram, use firstPage/nextPage
            // U8G2_SSD1306_128X64_NONAME_2_HW_I2C _u8g2; //2=uc-ram, two pages in ram, 2x times faster than 1
            // U8G2_SSD1306_128X64_NONAME_F_HW_I2C _u8g2; //F=Full-display frame buffer. clearBuffer/sendBuffer

            _u8g2 = u8g2;
            _u8g2.setFont(u8g2_font_guildenstern_nbp_tr);
            _u8g2.begin();
        };

        void DrawChart(char title[], byte values[])
        {
            // draw the basics of the chart;
            // int arraySize = sizeof(values);
            // if (arraySize < 0) arraySize = 60;
            // if (arraySize > 120) arraySize = 120;
            int arraySize = 60;

            _u8g2.firstPage();  
            do 
            {
                // display the title in the top;
                _u8g2.setFont(u8g2_font_inr16_mr); // 16px height
                _u8g2.drawStr(0, 16, title);

                // draw values 0-4 on Y-axis (left-side)
                _u8g2.setFont(u8g2_font_6x10_tn); // 8px, numeric only
                _u8g2.setCursor(0, 64 - 8); _u8g2.print(0);
                _u8g2.setCursor(0, 64 - (8 + 10)); _u8g2.print(1);
                _u8g2.setCursor(0, 64 - (8 + 20)); _u8g2.print(2);
                _u8g2.setCursor(0, 64 - (8 + 30)); _u8g2.print(3);
                _u8g2.setCursor(0, 64 - (8 + 40)); _u8g2.print(4);
                
                // draw time values on X-axis (bottom) //elke 3uur = 12kwartier = 24px
                _u8g2.setCursor(8, 64); _u8g2.print(6);
                _u8g2.setCursor(8 + 24, 64); _u8g2.print(9);
                _u8g2.setCursor(8 + 48, 64); _u8g2.print(12);
                _u8g2.setCursor(8 + 72, 64); _u8g2.print(15);
                _u8g2.setCursor(8 + 96, 64); _u8g2.print(18);
                _u8g2.setCursor(8 + 120 - 16, 64); _u8g2.print(21);

                // draw X and Y axis;
                _u8g2.drawLine(marginLeft, marginTop, marginLeft, marginTop + height);
                _u8g2.drawLine(marginLeft, marginTop + height, marginLeft + width, marginTop + height);

                // plot the chart;
                int prevPointX = marginLeft;
                int prevPointY = marginTop + height;
                for (int i = 0; i < arraySize; i++) 
                {
                    int newPointX = (i * (width / arraySize)) + marginLeft;
                    int newPointY = 64 - marginBottom - values[i];
                
                    // draw a line from prevPoint to newPoint;
                    _u8g2.drawLine(prevPointX, prevPointY, newPointX, newPointY);
                    
                    prevPointX = newPointX;
                    prevPointY = newPointY;
                }
            } while (_u8g2.nextPage());
        };
};
