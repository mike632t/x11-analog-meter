 /*
 * x11-analog-meter.c
 *
 * Copyright(C) 2025   MT
 *
 * Draws a basic analogue gauge to a oversized pixmap and resizes it  using
 * a bilinear filter when it is copied to the window.  This purpose of this 
 * technique being to produce a display with smoother less jagged lines.
 *
 * This  program is free software: you can redistribute it and / or modify it
 * under  the terms of the GNU General Public License as published  by  the
 * Free  Software Foundation, either version 3 of the License, or (at  your
 * option) any later version.
 *
 * This  program  is distributed in the hope that it will  be  useful,  but
 * WITHOUT   ANY   WARRANTY;   without even   the   implied   warranty   of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You  should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * 22 Jul 25  0.1.0001  - Initial version - MT
 * 
 * To Do                - Remove superfluous code 
 *                      - Rename variables 
 *                      - Tidy up colours
 *                      - Fix up scaling (the tick size is fixed and should
 *                        be a proportional)
 * 
 */ 

#define  NAME           "x11-draw" 
#define  VERSION        "0.1"
#define  BUILD          "0001"
#define  DATE           "21 Sep 25"
#define  AUTHOR         "MT"

#define  NO_GAMMA       2.2

#include <errno.h>       /* errno */ 

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>      /* vargs(), etc */ 
#include <math.h>
#include <unistd.h>
#include <sys/types.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "gcc-debug.h"
#include "gcc-wait.h"

#define WIDTH 600
#define HEIGHT 300
#define SCALE 1

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#if defined (GAMMA)

#define GAMMA_LUT_SIZE 256
static float f_gamma_decode[GAMMA_LUT_SIZE];
static unsigned char f_gamma_encode[GAMMA_LUT_SIZE];

static void v_init_gamma_lookup_table(void)  /* Initialize lookup tables once */ 
{
   float f_linear;
   int i_count;
   
   for ( i_count = 0; i_count < GAMMA_LUT_SIZE; ++i_count) {
      f_linear = powf((float)i_count / 255.0f, GAMMA);
      f_gamma_decode[i_count] = f_linear;
      f_gamma_encode[i_count] = (unsigned char)(powf(f_linear, 1.0f / GAMMA) * 255.0f + 0.5f);
   }
}

static unsigned long bilinear_sample(const XImage *x_image,
                     float f_x_pos, float f_y_pos,
                     unsigned long i_red_mask, unsigned long i_green_mask, unsigned long i_blue_mask,
                     int i_red_shift, int i_green_shift, int i_blue_shift)  /* Returns a bilinear sample of image at (x, y) using masks/shifts with gamma correction */ 
{
   int i_x_pos = (int)f_x_pos;
   int i_y_pos = (int)f_y_pos;
   int i_x_start = i_x_pos + 1 < x_image->width  ? i_x_pos + 1 : i_x_pos;
   int i_y_start = i_y_pos + 1 < x_image->height ? i_y_pos + 1 : i_y_pos;

   float ax = f_x_pos - (float)i_x_pos;
   float ay = f_y_pos - (float)i_y_pos;

   unsigned long p00 = XGetPixel((XImage *)x_image, i_x_pos, i_y_pos);
   unsigned long p10 = XGetPixel((XImage *)x_image, i_x_start, i_y_pos);
   unsigned long p01 = XGetPixel((XImage *)x_image, i_x_pos, i_y_start);
   unsigned long p11 = XGetPixel((XImage *)x_image, i_x_start, i_y_start);

   int r00 = (int)((p00 & i_red_mask) >> i_red_shift), r10 = (int)((p10 & i_red_mask) >> i_red_shift);
   int r01 = (int)((p01 & i_red_mask) >> i_red_shift), r11 = (int)((p11 & i_red_mask) >> i_red_shift);
   int g00 = (int)((p00 & i_green_mask) >> i_green_shift), g10 = (int)((p10 & i_green_mask) >> i_green_shift);
   int g01 = (int)((p01 & i_green_mask) >> i_green_shift), g11 = (int)((p11 & i_green_mask) >> i_green_shift);
   int b00 = (int)((p00 & i_blue_mask) >> i_blue_shift), b10 = (int)((p10 & i_blue_mask) >> i_blue_shift);
   int b01 = (int)((p01 & i_blue_mask) >> i_blue_shift), b11 = (int)((p11 & i_blue_mask) >> i_blue_shift);

    /* Decode to linear light */ 
   float r0 = f_gamma_decode[r00] + ax * (f_gamma_decode[r10] - f_gamma_decode[r00]);
   float r1 = f_gamma_decode[r01] + ax * (f_gamma_decode[r11] - f_gamma_decode[r01]);
   float g0 = f_gamma_decode[g00] + ax * (f_gamma_decode[g10] - f_gamma_decode[g00]);
   float g1 = f_gamma_decode[g01] + ax * (f_gamma_decode[g11] - f_gamma_decode[g01]);
   float b0 = f_gamma_decode[b00] + ax * (f_gamma_decode[b10] - f_gamma_decode[b00]);
   float b1 = f_gamma_decode[b01] + ax * (f_gamma_decode[b11] - f_gamma_decode[b01]);

    /* Interpolate vertically */ 
   float r_lin = r0 + ay * (r1 - r0);
   float g_lin = g0 + ay * (g1 - g0);
   float b_lin = b0 + ay * (b1 - b0);

    /* Encode back to gamma space */ 
   int r = f_gamma_encode[(int)(r_lin * 255.0f + 0.5f)];
   int g = f_gamma_encode[(int)(g_lin * 255.0f + 0.5f)];
   int b = f_gamma_encode[(int)(b_lin * 255.0f + 0.5f)];

   return (((unsigned long)r << i_red_shift) & i_red_mask)
       | (((unsigned long)g << i_green_shift) & i_green_mask)
       | (((unsigned long)b << i_blue_shift) & i_blue_mask);
}
#else
 
static unsigned long bilinear_sample(const XImage *x_image, float f_x_pos, float f_y_pos,
                     unsigned long i_red_mask, unsigned long i_green_mask, unsigned long i_blue_mask,
                     int i_red_shift, int i_green_shift, int i_blue_shift)  /* Returns a bilinear sample of image at (x, y) using masks/shifts */ 
{
   int i_x_pos = (int)f_x_pos;
   int i_y_pos = (int)f_y_pos;
   int i_x_start = i_x_pos + 1 < x_image->width  ? i_x_pos + 1 : i_x_pos;
   int i_y_start = i_y_pos + 1 < x_image->height ? i_y_pos + 1 : i_y_pos;

   float ax = f_x_pos - (float)i_x_pos;
   float ay = f_y_pos - (float)i_y_pos;

   unsigned long p00 = XGetPixel((XImage *)x_image, i_x_pos, i_y_pos);
   unsigned long p10 = XGetPixel((XImage *)x_image, i_x_start, i_y_pos);
   unsigned long p01 = XGetPixel((XImage *)x_image, i_x_pos, i_y_start);
   unsigned long p11 = XGetPixel((XImage *)x_image, i_x_start, i_y_start);

   int r00 = (int)((p00 & i_red_mask) >> i_red_shift), r10 = (int)((p10 & i_red_mask) >> i_red_shift);
   int r01 = (int)((p01 & i_red_mask) >> i_red_shift), r11 = (int)((p11 & i_red_mask) >> i_red_shift);
   int g00 = (int)((p00 & i_green_mask) >> i_green_shift), g10 = (int)((p10 & i_green_mask) >> i_green_shift);
   int g01 = (int)((p01 & i_green_mask) >> i_green_shift), g11 = (int)((p11 & i_green_mask) >> i_green_shift);
   int b00 = (int)((p00 & i_blue_mask) >> i_blue_shift), b10 = (int)((p10 & i_blue_mask) >> i_blue_shift);
   int b01 = (int)((p01 & i_blue_mask) >> i_blue_shift), b11 = (int)((p11 & i_blue_mask) >> i_blue_shift);

   float r0 = r00 + ax * (r10 - r00);
   float r1 = r01 + ax * (r11 - r01);
   float g0 = g00 + ax * (g10 - g00);
   float g1 = g01 + ax * (g11 - g01);
   float b0 = b00 + ax * (b10 - b00);
   float b1 = b01 + ax * (b11 - b01);

   int r = (int)(r0 + ay * (r1 - r0) + 0.5f);
   int g = (int)(g0 + ay * (g1 - g0) + 0.5f);
   int b = (int)(b0 + ay * (b1 - b0) + 0.5f);

   return (((unsigned long)r << i_red_shift) & i_red_mask)
       | (((unsigned long)g << i_green_shift) & i_green_mask)
       | (((unsigned long)b << i_blue_shift) & i_blue_mask);
}
#endif

static int mask_shift(unsigned long i_mask) /* Helper: compute left shift from mask */ 
{
   int s = 0;
   if (!i_mask) return 0;
   while ((i_mask & 1UL) == 0UL) {
      i_mask >>= 1;
      s++;
   }
   return s;
}

void XScaleArea(Display *x_display, Drawable x_source, Drawable x_dest, GC x_context,
          int i_source_xoffset, int i_source_yoffset, unsigned int i_source_width, unsigned int i_source_height,
          int i_dest_x_offset, int i_dest_y_offset, unsigned int i_dest_width, unsigned int i_dest_height)
{

   XImage *x_dest_image, *x_source_img ; 
   Visual *x_visual;
   float f_y_pos, f_x_pos, f_scale_x, f_scale_y;
   unsigned long i_red_mask, i_green_mask, i_blue_mask;
   unsigned long pix;
   unsigned int i_x_pos, i_y_pos;
   int i_red_shift, i_green_shift, i_blue_shift;
   int i_screen, i_depth, i_pad;


   i_screen = DefaultScreen(x_display);
   i_depth = DefaultDepth(x_display, i_screen);
   i_pad = BitmapPad(x_display);
   x_visual = DefaultVisual(x_display, i_screen);
   
   if (i_source_width == 0 || i_source_height == 0 || i_dest_width == 0 || i_dest_height == 0) error(errno, "Invalid dimensions");
   
   x_source_img = XGetImage(x_display, x_source, i_source_xoffset, i_source_yoffset, i_source_width, i_source_height, AllPlanes, ZPixmap);
   if (!x_source_img) error(errno, "XGetImage failed");

   x_dest_image = XCreateImage(x_display, x_visual, i_depth, ZPixmap, 0, NULL, i_dest_width, i_dest_height, i_pad, 0);
   if (!x_dest_image) 
   {
      XDestroyImage(x_source_img);
      error(errno, "XCreateImage failed");
   }

   x_dest_image->data = (char *)malloc(x_dest_image->bytes_per_line * x_dest_image->height);
   if (!x_dest_image->data) 
   {
      XDestroyImage(x_dest_image);
      XDestroyImage(x_source_img);
      error(errno, "malloc failed");
   }

   i_red_mask = x_visual->red_mask;
   i_green_mask = x_visual->green_mask;
   i_blue_mask = x_visual->blue_mask;

   i_red_shift = mask_shift(i_red_mask);
   i_green_shift = mask_shift(i_green_mask);
   i_blue_shift = mask_shift(i_blue_mask);

   f_scale_x = (i_dest_width > 1 && i_source_width > 1) ? (float)(i_source_width - 1) / (float)(i_dest_width - 1) : 0.0f;
   f_scale_y = (i_dest_height > 1 && i_source_height > 1) ? (float)(i_source_height - 1) / (float)(i_dest_height - 1) : 0.0f;

#if defined (GAMMA)
   v_init_gamma_lookup_table();  
#endif

   for (i_y_pos = 0; i_y_pos < i_dest_height; ++i_y_pos) {
      f_y_pos = f_scale_y * (float)i_y_pos;
      for (i_x_pos = 0; i_x_pos < i_dest_width; ++i_x_pos) {
       f_x_pos = f_scale_x * (float)i_x_pos;
       pix = bilinear_sample(x_source_img, f_x_pos, f_y_pos, i_red_mask, i_green_mask, i_blue_mask, i_red_shift, i_green_shift, i_blue_shift);
       XPutPixel(x_dest_image, i_x_pos, i_y_pos, pix);
      }
   }

   XPutImage(x_display, x_dest, x_context, x_dest_image, 0, 0, i_dest_x_offset, i_dest_y_offset, i_dest_width, i_dest_height);
   XSync(x_display, False);

   XDestroyImage(x_source_img);
   XDestroyImage(x_dest_image);
}

void draw_gauge(Display *x_display, Pixmap x_pixmap, GC x_context, int i_max,  int i_step, float f_value, int i_weight) 
{
   Window x_window;
   double f_angle;
   unsigned int i_width, i_height, i_radius;
   unsigned int i_border, i_temp;
   int i_top, i_left, i_angle, i_count;
   int i_x_start, i_y_start, i_x_end, i_y_end, i_x_pos, i_y_pos;
   
   XGetGeometry(x_display, x_pixmap, &x_window, &i_top, &i_left, &i_width, &i_height, &i_temp, &i_temp );  /* Get pixmap size */ 
   debug(printf("%ux%u+%u+%u\n", i_width, i_height, i_left, i_top));

   XSetForeground(x_display, x_context, 0xd3d3d3);  /* Set colour - bit of a fudge! */ 

   XFillRectangle(x_display, x_pixmap, x_context, 0, 0, i_width, i_height);  /* Fill pixmap with current foreground colour */ 

   XSetForeground(x_display, x_context, 0x000000);  /* Set colour - bit of a fudge! */ 

   XSetLineAttributes(x_display, x_context, SCALE , LineSolid, CapRound, JoinRound);  /* Set line attributes */ 

   XDrawRectangle(x_display, x_pixmap, x_context, 0, 0, i_width, i_height);  /* Fill pixmap with current foreground colour */ 
   
   i_border = 10 * SCALE;
   i_radius = MIN(i_width / 2, i_height) - i_border;

   XSetForeground(x_display, x_context, 0xfffff0);  /* Set colour - bit of a fudge! */ 

   XFillArc(x_display, x_pixmap, x_context,  /* Draw semicircular arc */ 
      (i_width / 2 - i_radius), (i_height - i_radius - SCALE), i_radius * 2, i_radius * 2, 0, 180 * 64);

   XSetForeground(x_display, x_context, 0x000000);  /* Set colour - bit of a fudge! */ 

   XDrawArc(x_display, x_pixmap, x_context,  /* Draw semicircular arc */ 
      (i_width / 2 - i_radius), (i_height - i_radius - SCALE), i_radius * 2, i_radius * 2, 0, 180 * 64);


   XSetLineAttributes(x_display, x_context, i_weight * SCALE , LineSolid, CapRound, JoinRound);  /* Set line attributes */ 
   i_weight = i_weight / 2;
   i_radius = MIN(i_width / 2, i_height) - i_border * 2;

   XSetForeground(x_display, x_context, BlackPixel(x_display, DefaultScreen(x_display)));  /* Set line colour */  

   XDrawArc(x_display, x_pixmap, x_context, (i_width / 2 - i_radius), (i_height - i_radius - i_weight * SCALE), i_radius * 2, i_radius * 2, 0, 180 * 64);  /* Draw semicircular arc */ 

   for (i_count = 0; i_count <= i_max; i_count += (i_max / i_step))  /* Draw tick marks on scale */ 
   {
      i_angle = (i_count * 180) / i_max;
      f_angle = i_angle * M_PI / 180.0;
      i_x_start = (i_width / 2) + cos(f_angle) * (i_radius - 10 * SCALE);
      i_y_start = (i_height - i_weight * SCALE) - sin(f_angle) * (i_radius - 10 * SCALE);
      i_x_end = (i_width / 2) + cos(f_angle) * i_radius;
      i_y_end = (i_height - i_weight * SCALE) - sin(f_angle) * i_radius;
      XDrawLine(x_display, x_pixmap, x_context, i_x_start, i_y_start, i_x_end, i_y_end);
   }

   XSetForeground(x_display, x_context, 0xa90000);  /* Set pointer colour - bit of a fudge! */ 
   
   f_angle = (f_value * 180.0) / (float)i_max;
   f_angle = f_angle * M_PI / 180.0;
   i_x_pos = (i_width / 2) - cos(f_angle) * (i_radius - 20 * SCALE);
   i_y_pos = (i_height - (i_weight * 2 * SCALE)) - sin(f_angle) * (i_radius - 20 * SCALE);
   XDrawLine(x_display, x_pixmap, x_context, (i_width / 2), (i_height - (i_weight * 2 * SCALE)), i_x_pos, i_y_pos);  /* Draw pointer */ 

   i_radius = i_radius >> 4;  /* Divide by 16 */
   XFillArc(x_display, x_pixmap, x_context, (i_width / 2 - i_radius), (i_height - i_radius -  (1 * SCALE)), i_radius * 2, i_radius * 2, 0, 180 * 64);  /* Draw semicircular boss */ 
}

float f_get_cpu_temp()
{
   FILE *h_file;
   int i_temp;
   
   h_file = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
   if (h_file) 
   {
      fscanf(h_file, "%d", &i_temp);
      fclose(h_file);
   }
   else
      i_temp = 0;
   return (float)(i_temp / 1000); /*  Convert to °C */
}


int main() 
{
   Display *x_display;
   Window x_window;
   XColor x_foreground_colour, x_background_colour, x_exact_colour;
   XEvent x_event;
   GC x_context;
   Atom wm_delete_window;
   Pixmap x_pixmap;

   static const char* s_foreground = "black";
   char* s_background = "ivory";
      
   float f_value, f_inc;
   unsigned int i_width = WIDTH * 0.75 , i_height = HEIGHT * 0.75, i_depth;
   int i_screen;

   int b_continue;
    
   if (!(x_display = XOpenDisplay(NULL)))
      error (errno, "could not open the default display");  /* Open the display and create a new window */ 
   i_screen = DefaultScreen (x_display);
   i_depth = DefaultDepth (x_display, i_screen);
   x_window = XCreateSimpleWindow (x_display,  XDefaultRootWindow(x_display), 100, 100, WIDTH, HEIGHT, 0, BlackPixel(x_display, i_screen), WhitePixel(x_display, i_screen));

   if (!XAllocNamedColor (x_display, DefaultColormap (x_display, i_screen), s_background, &x_background_colour, &x_exact_colour))
      error(errno, "Failed to allocate colour '%s'", s_background);

   if (!XAllocNamedColor (x_display, DefaultColormap (x_display, i_screen), s_foreground, &x_foreground_colour, &x_exact_colour))
      error(errno, "Failed to allocate colour '%s'", s_foreground);

   x_context = XCreateGC(x_display, x_window, 0, NULL);
      
   wm_delete_window = XInternAtom(x_display, "WM_DELETE_WINDOW", False);
   XSetWMProtocols(x_display, x_window, &wm_delete_window, i_depth);
   XSelectInput(x_display, x_window, ExposureMask | KeyPressMask);
   XMapWindow(x_display, x_window);

   x_pixmap = XCreatePixmap(x_display, RootWindow(x_display, i_screen), i_width * SCALE, i_height * SCALE, DefaultDepth(x_display, i_screen));

   XSetForeground(x_display, x_context, x_background_colour.pixel);

   printf("SCALE = %d\n", SCALE);

   f_value = 0.0;
   f_inc = 0.1;
   b_continue = True;
   while (b_continue) 
   {
      draw_gauge(x_display, x_pixmap, x_context, 100, 20, f_value, 3);  /* Draw gauge on pixmap */ 
      if (SCALE == 1)
         XCopyArea(x_display, x_pixmap, x_window, x_context, 0, 0, i_width * SCALE, i_height * SCALE, WIDTH * 0.125, HEIGHT * 0.125); /* Just copy bitmap */
      else
         XScaleArea(x_display, x_pixmap, x_window, x_context, 0, 0, i_width * SCALE, i_height * SCALE, WIDTH * 0.125, HEIGHT * 0.125, i_width, i_height); /* Use bilinear filter */
      f_value += f_inc;
      if ((f_value >= 100.0) || (f_value <= 0.0)) f_inc = f_inc * -1; 
      while (XPending(x_display) > 0) 
      {
         XNextEvent(x_display, &x_event);
         switch (x_event.type) 
         {
            case Expose: 
                /** XCopyArea(x_display, x_pixmap, x_window, x_context, 0, 0, i_width * SCALE, i_height * SCALE, 0, 0); */ 
               break;         
            case KeyPress: 
               b_continue = False;
               break;
            case ClientMessage: 
               if ((Atom) x_event.xclient.data.l[0] == wm_delete_window)
                  b_continue = False;
            break;
         }
      }
      i_wait(5);
   }

   XFreePixmap(x_display, x_pixmap);
   XCloseDisplay(x_display);
   return 0;
}
