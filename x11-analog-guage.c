
/*
 * x11-anslog-guage.c - An analogue meter
 *
 * Displays an analogue meter.
 *
 * This  program is free software: you can redistribute it and/or modify it
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
 * 03 Nov 26   0.1   - Initial version - MT
 *
 */

#define NAME           "x11-analog-guage"
#define VERSION        "0.1"
#define BUILD          "0001"
#define AUTHOR         "MT"

#define WIDTH          500
#define HEIGHT         320
#define PADDING        20

#define MIN_VALUE      0
#define MAX_VALUE      100

#define START_ANGLE    15.0
#define END_ANGLE      165.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#define PI 3.14159265358979323846

/* Initial indicated value */

static int i_value = 50;


/*
 * l_named_colour_pixel() - Allocate a named X11 colour
 *
 * Attempts to allocate the requested colour from the default
 * colour map. If the colour cannot be allocated, the supplied
 * default pixel value is returned.
 *
 */

static unsigned long l_named_colour_pixel(
   Display *x_display,
   Colormap x_colourmap,
   const char *s_name,
   unsigned long l_default)
{
   XColor x_colour;

   if (XParseColor(
      x_display, x_colourmap, s_name, &x_colour))
   {
      if (XAllocColor(x_display, x_colourmap, &x_colour))
         return x_colour.pixel;
   }

   fprintf(stderr, "%s: Unable to allocate colour %s\n",
      NAME, s_name);

   return l_default;
}


/*
 * d_radians() - Convert degrees to radians
 *
 * The trigonometric functions in the C maths library expect
 * angles to be specified in radians rather than degrees.
 *
 */

static double d_radians(double d_angle)
{
   return d_angle * PI / 180.0;
}


/*
 * d_value_angle() - Calculate the angle for a scale value
 *
 *  Zero is positioned at END_ANGLE (165 degrees).
 *  100 is positioned at START_ANGLE (15 degrees).
 *
 *  The scale therefore increases from left to right.
 *
 */

static double d_value_angle(int i_value)
{
   return END_ANGLE -
      ((double)(i_value - MIN_VALUE) /
      (double)(MAX_VALUE - MIN_VALUE)) *
      (END_ANGLE - START_ANGLE);
}


/*
 * v_point() - Calculate a point on the dial
 *
 * The centre of the dial is used as the origin.
 *
 * The X coordinate increases towards the right and the Y
 * coordinate increases downwards in the X11 coordinate system.
 * Consequently the sine calculation is subtracted from the
 * centre Y coordinate.
 *
 */

static void v_point(
   int i_centre_x,
   int i_centre_y,
   double d_radius,
   double d_angle,
   int *i_x,
   int *i_y)
{
   double d_angle_rad;

   d_angle_rad = d_radians(d_angle);

   *i_x = i_centre_x +
      (int)(d_radius * cos(d_angle_rad) + 0.5);

   *i_y = i_centre_y -
      (int)(d_radius * sin(d_angle_rad) + 0.5);
}


/*
 * v_draw_text() - Draw centred text
 *
 * XDrawString() positions text using the left-hand end of
 * the text baseline. XTextWidth() is used to calculate the
 * width so that the text can be centred on the X coordinate.
 *
 */

static void v_draw_text(
   Display *x_display,
   Drawable x_buffer,
   GC gc_text,
   XFontStruct *x_font,
   int i_x,
   int i_y,
   const char *s_text)
{
   int i_width;

   i_width = XTextWidth(
      x_font, s_text, (int)strlen(s_text));

   XDrawString(
      x_display, x_buffer, gc_text,
      i_x - i_width / 2,
      i_y,
      s_text,
      (int)strlen(s_text));
}


/*
 * i_dial_radius() - Calculate the dial radius
 *
 * The dial must fit within both the window width and height.
 * The height calculation reserves space beneath the dial
 * for the displayed value.
 *
 */

static int i_dial_radius(int i_width, int i_height)
{
   int i_radius;

   i_radius = i_width / 2 - PADDING;

   if (i_radius > i_height - 65)
      i_radius = i_height - 65;

   return i_radius;
}


/*
 * v_draw_dial() - Draw the semi-circular dial
 *
 * Draws the filled dial face, outer arc, horizontal baseline,
 * graduations and numerical labels.
 *
 * There is one tick for every scale value from 0 to 100.
 *
 * Major ticks are drawn every 10 units, intermediate ticks
 * every 5 units and minor ticks at all remaining positions.
 *
 */

static void v_draw_dial(
   Display *x_display,
   Drawable x_buffer,
   GC gc_face,
   GC gc_scale,
   GC gc_needle,
   XFontStruct *x_font,
   int i_window_width,
   int i_window_height)
{
   int i_centre_x, i_centre_y, i_radius;
   int i_x1, i_y1, i_x2, i_y2;
   int i_tick_length, i_thickness;
   int i_label_x, i_label_y;
   int i_count;

   char s_label[8];

   i_centre_x = i_window_width / 2;
   i_centre_y = i_window_height - PADDING - 10;

   i_radius = i_dial_radius(
      i_window_width, i_window_height);

   if (i_radius <= 0)
      return;

   /* Fill the semi-circular dial face */
   
   XFillArc(
      x_display, x_buffer, gc_face,
      i_centre_x - i_radius,
      i_centre_y - i_radius,
      i_radius * 2,
      i_radius * 2,
      0, 180 * 64);

   /** XFillArc(
      x_display, x_buffer, gc_face,
      i_centre_x - i_radius,
      i_centre_y - i_radius,
      i_radius * 2,
      i_radius * 2,
      (int)((START_ANGLE -3 ) * 64),
      (int)((END_ANGLE - START_ANGLE + 6) * 64)); */

   /* Draw the outer arc */

   XSetLineAttributes(
      x_display, gc_scale,
      2, LineSolid, CapRound, JoinRound);

   XDrawArc(
      x_display, x_buffer, gc_scale,
      i_centre_x - i_radius,
      i_centre_y - i_radius,
      i_radius * 2,
      i_radius * 2,
      /** (int)(START_ANGLE * 64),
      (int)((END_ANGLE - START_ANGLE) * 64)); */
      (int)(0 * 64),
      (int)((180 - 0) * 64));
      
   /* Draw the central pivot */

   XFillArc(
      x_display, x_buffer, gc_needle,
      i_centre_x - 12,
      i_centre_y - 12,
      24, 24,
      (int)(0 * 64),
      (int)((180) * 64)); 
      
   /* Draw the horizontal baseline (zero-degree reference) */

   XSetLineAttributes(
      x_display, gc_scale,
      2, LineSolid, CapRound, JoinRound);

   XDrawLine(
      x_display, x_buffer, gc_scale,
      i_centre_x - i_radius,
      i_centre_y,
      i_centre_x + i_radius,
      i_centre_y);

   /* Draw the scale graduations */

   for (i_count = MIN_VALUE; i_count <= MAX_VALUE; ++i_count)
   {
      double d_angle;

      d_angle = d_value_angle(i_count);

      if ((i_count % 10) == 0 ||
          i_count == MIN_VALUE ||
          i_count == MAX_VALUE)
      {
         /* Major tick mark */

         i_tick_length = i_radius / 10;
         i_thickness = (i_radius + 24) / 48;

         if (i_thickness < 2)
            i_thickness = 2;
      }
      else if ((i_count % 5) == 0)
      {
         /* Intermediate tick mark */

         i_tick_length = i_radius / 15;
         i_thickness = (i_radius + 32) / 64;

         if (i_thickness < 1)
            i_thickness = 1;
      }
      else
      {
         /* Minor tick mark */

         i_tick_length = i_radius / 25;
         i_thickness = 1;
      }

      /* Calculate the outer end of the tick */

      v_point(
         i_centre_x, i_centre_y,
         i_radius - i_radius / 64,
         d_angle,
         &i_x1, &i_y1);

      /* Calculate the inner end of the tick */

      v_point(
         i_centre_x, i_centre_y,
         i_radius - i_tick_length,
         d_angle,
         &i_x2, &i_y2);

      XSetLineAttributes(
         x_display, gc_scale,
         i_thickness,
         LineSolid, CapRound, JoinRound);

      XDrawLine(
         x_display, x_buffer, gc_scale,
         i_x1, i_y1, i_x2, i_y2);

      /*
       * Draw numerical labels at 10-unit intervals.
       * The minimum and maximum values are also labelled.
       */

      if ((i_count % 10) == 0 ||
          i_count == MIN_VALUE ||
          i_count == MAX_VALUE)
      {
         v_point(
            i_centre_x, i_centre_y,
            i_radius - i_tick_length - 18,
            d_angle,
            &i_label_x, &i_label_y);

         snprintf(
            s_label, sizeof(s_label),
            "%d", i_count);

         v_draw_text(
            x_display, x_buffer,
            gc_scale, x_font,
            i_label_x,
            i_label_y + 4,
            s_label);
      }
   }
}


/*
 * v_draw_needle() - Draw the indicator needle
 *
 * The needle angle is calculated from the current scale value.
 * Its length is 88% of the dial radius.
 *
 * The needle is drawn using OrangeRed to distinguish it from
 * the Gray49 scale markings.
 *
 * A small circular pivot is drawn at the centre of the dial.
 *
 */

static void v_draw_needle(
   Display *x_display,
   Drawable x_buffer,
   GC gc_needle,
   int i_window_width,
   int i_window_height,
   int i_value)
{
   int i_centre_x, i_centre_y, i_radius;
   int i_x, i_y;
   int i_thickness;
   double d_angle;

   i_centre_x = i_window_width / 2;
   i_centre_y = i_window_height - PADDING - 10;

   i_radius = i_dial_radius(
      i_window_width, i_window_height);

   if (i_radius <= 0)
      return;

   /* Calculate the needle angle */

   d_angle = d_value_angle(i_value);

   /* Calculate the needle endpoint */

   v_point(
      i_centre_x, i_centre_y,
      i_radius * 0.88,
      d_angle,
      &i_x, &i_y);

   /* Draw the needle */

   i_thickness = i_radius / 50;

   if (i_thickness < 2)
      i_thickness = 2;

   XSetLineAttributes(
      x_display, gc_needle,
      i_thickness,
      LineSolid, CapRound, JoinRound);

   XDrawLine(
      x_display, x_buffer, gc_needle,
      i_centre_x, i_centre_y - 4,
      i_x, i_y);

   /* Draw the central pivot 

   XFillArc(
      x_display, x_buffer, gc_needle,
      i_centre_x - 6,
      i_centre_y - 6,
      12, 12,
      0, 360 * 64); */
}


/*
 * v_draw_value() - Display the current scale value
 *
 * The indicated value is displayed beneath the dial.
 *
 */

static void v_draw_value(
   Display *x_display,
   Drawable x_buffer,
   GC gc_scale,
   XFontStruct *x_font,
   int i_window_width,
   int i_window_height)
{
   char s_label[32];

   snprintf(
      s_label, sizeof(s_label), "%d", i_value);

   v_draw_text(
      x_display, x_buffer,
      gc_scale, x_font,
      i_window_width / 2,
      i_window_height - 5,
      s_label);
}


/*
 * v_draw() - Draw the complete instrument
 *
 * The dial and needle are drawn into an off-screen pixmap.
 * The completed image is then copied to the display window.
 *
 * This avoids displaying partially drawn graphics.
 *
 */

static void v_draw(
   Display *x_display,
   Window x_window,
   Pixmap x_buffer,
   GC gc_background,
   GC gc_face,
   GC gc_scale,
   GC gc_needle,
   XFontStruct *x_font,
   int i_window_width,
   int i_window_height)
{
   /* Clear the off-screen drawing buffer */

   XFillRectangle(
      x_display, x_buffer, gc_background,
      0, 0,
      i_window_width,
      i_window_height);

   /* Draw the dial face and graduations */

   v_draw_dial(
      x_display, x_buffer,
      gc_face, gc_scale, gc_needle,
      x_font,
      i_window_width,
      i_window_height);

   /* Draw the indicator needle */

   v_draw_needle(
      x_display, x_buffer,
      gc_needle,
      i_window_width,
      i_window_height,
      i_value);

   /* Draw the numerical value */

   v_draw_value(
      x_display, x_buffer,
      gc_scale, x_font,
      i_window_width,
      i_window_height);

   /* Copy the completed image to the display window */

   XCopyArea(
      x_display, x_buffer, x_window,
      gc_background,
      0, 0,
      i_window_width,
      i_window_height,
      0, 0);

   XFlush(x_display);
}


/*
 * main() - Main program
 *
 * Opens the X display, creates the window and allocates the
 * graphics contexts used to draw the instrument.
 *
 * Keyboard events are used to adjust the indicated value.
 *
 */

int main(int argc, char *argv[])
{
   Display *x_display;
   Window x_window;
   Pixmap x_buffer;
   XEvent x_event;

   XFontStruct *x_font;

   GC gc_background;
   GC gc_face;
   GC gc_scale;
   GC gc_needle;

   Colormap x_colourmap;

   unsigned int i_window_width = WIDTH;
   unsigned int i_window_height = HEIGHT;
   unsigned int i_colour_depth;

   unsigned long l_white;
   unsigned long l_flora_white;
   unsigned long l_gray;
   unsigned long l_orange_red;

   int i_screen;
   int b_abort = False;

   (void)argc;
   (void)argv;

   /* Open the X display */

   if (!(x_display = XOpenDisplay(NULL)))
   {
      fprintf(stderr,
         "%s: Unable to open X display\n", NAME);

      return EXIT_FAILURE;
   }

   i_screen = DefaultScreen(x_display);

   i_colour_depth = DefaultDepth(
      x_display, i_screen);

   x_colourmap = DefaultColormap(
      x_display, i_screen);

   /* Get the default display colours */

   l_white = WhitePixel(x_display, i_screen);

   /* Allocate the colours used by the instrument */

   l_flora_white = l_named_colour_pixel(
      x_display, x_colourmap,
      "FloralWhite", l_white);

   l_gray = l_named_colour_pixel(
      x_display, x_colourmap,
      "Gray49", BlackPixel(x_display, i_screen));

   l_orange_red = l_named_colour_pixel(
      x_display, x_colourmap,
      "OrangeRed", BlackPixel(x_display, i_screen));

   /*
    * Create the display window.
    *
    * The window background is white, as in the original clock.
    *
    */

   x_window = XCreateSimpleWindow(
      x_display,
      RootWindow(x_display, i_screen),
      100, 100,
      i_window_width,
      i_window_height,
      0,
      BlackPixel(x_display, i_screen),
      l_white);

   if (!x_window)
   {
      fprintf(stderr,
         "%s: Unable to create window\n", NAME);

      XCloseDisplay(x_display);
      return EXIT_FAILURE;
   }

   /* Set the window title */

   XStoreName(
      x_display, x_window,
      "X11 Semi-Circular Analogue Dial");

   /* Select the events required by the program */

   XSelectInput(
      x_display, x_window,
      ExposureMask |
      StructureNotifyMask |
      KeyPressMask);

   /* Map the window onto the display */

   XMapWindow(x_display, x_window);

   /* Create the graphics contexts */

   gc_background = XCreateGC(
      x_display, x_window, 0, NULL);

   gc_face = XCreateGC(
      x_display, x_window, 0, NULL);

   gc_scale = XCreateGC(
      x_display, x_window, 0, NULL);

   gc_needle = XCreateGC(
      x_display, x_window, 0, NULL);

   if (!gc_background || !gc_face ||
       !gc_scale || !gc_needle)
   {
      fprintf(stderr,
         "%s: Unable to create graphics context\n", NAME);

      XCloseDisplay(x_display);
      return EXIT_FAILURE;
   }

   /*
    * Set the foreground colours for each graphics context.
    *
    * FloralWhite - semi-circular dial face
    * Gray49      - arc, graduations and numerical labels
    * OrangeRed   - indicator needle and central pivot
    * White       - window background
    */

   XSetForeground(
      x_display, gc_background, l_white);

   XSetForeground(
      x_display, gc_face, l_flora_white);

   XSetForeground(
      x_display, gc_scale, l_gray);

   XSetForeground(
      x_display, gc_needle, l_orange_red);

   /* Load the standard X11 fixed-width font */

   x_font = XLoadQueryFont(
      x_display, "fixed");

   if (!x_font)
   {
      fprintf(stderr,
         "%s: Unable to load font\n", NAME);

      XFreeGC(x_display, gc_needle);
      XFreeGC(x_display, gc_scale);
      XFreeGC(x_display, gc_face);
      XFreeGC(x_display, gc_background);

      XDestroyWindow(x_display, x_window);
      XCloseDisplay(x_display);

      return EXIT_FAILURE;
   }

   XSetFont(
      x_display, gc_scale, x_font->fid);

   /* Create the off-screen drawing buffer */

   x_buffer = XCreatePixmap(
      x_display, x_window,
      i_window_width,
      i_window_height,
      i_colour_depth);

   if (!x_buffer)
   {
      fprintf(stderr,
         "%s: Unable to create pixmap\n", NAME);

      XFreeFont(x_display, x_font);

      XFreeGC(x_display, gc_needle);
      XFreeGC(x_display, gc_scale);
      XFreeGC(x_display, gc_face);
      XFreeGC(x_display, gc_background);

      XDestroyWindow(x_display, x_window);
      XCloseDisplay(x_display);

      return EXIT_FAILURE;
   }

   /*
    * Main event loop.
    *
    * The window is redrawn whenever it is exposed or resized.
    * Keyboard input changes the indicated value.
    *
    */

   while (!b_abort)
   {
      XNextEvent(x_display, &x_event);

      switch (x_event.type)
      {
         case Expose:

            /* Redraw the window when it becomes visible */

            if (x_event.xexpose.count == 0)
            {
               v_draw(
                  x_display, x_window,
                  x_buffer,
                  gc_background,
                  gc_face,
                  gc_scale,
                  gc_needle,
                  x_font,
                  i_window_width,
                  i_window_height);
            }

            break;

         case ConfigureNotify:

            /* Check whether the window has been resized */

            if (i_window_width !=
                   (unsigned int)x_event.xconfigure.width ||
                i_window_height !=
                   (unsigned int)x_event.xconfigure.height)
            {
               i_window_width =
                  x_event.xconfigure.width;

               i_window_height =
                  x_event.xconfigure.height;

               /* Recreate the drawing buffer */

               XFreePixmap(x_display, x_buffer);

               x_buffer = XCreatePixmap(
                  x_display, x_window,
                  i_window_width,
                  i_window_height,
                  i_colour_depth);

               if (!x_buffer)
               {
                  fprintf(stderr,
                     "%s: Unable to resize pixmap\n", NAME);

                  b_abort = True;
               }
               else
               {
                  v_draw(
                     x_display, x_window,
                     x_buffer,
                     gc_background,
                     gc_face,
                     gc_scale,
                     gc_needle,
                     x_font,
                     i_window_width,
                     i_window_height);
               }
            }

            break;

         case KeyPress:
         {
            KeySym x_key;

            x_key = XLookupKeysym(
               &x_event.xkey, 0);

            /* Adjust the indicated value */

            switch (x_key)
            {
               case XK_Left:
                  --i_value;
                  break;

               case XK_Right:
                  ++i_value;
                  break;

               case XK_Up:
                  i_value += 10;
                  break;

               case XK_Down:
                  i_value -= 10;
                  break;

               case XK_Home:
                  i_value = MIN_VALUE;
                  break;

               case XK_End:
                  i_value = MAX_VALUE;
                  break;

               case XK_Escape:
                  b_abort = True;
                  break;
            }

            /* Restrict the value to the defined scale */

            if (i_value < MIN_VALUE)
               i_value = MIN_VALUE;

            if (i_value > MAX_VALUE)
               i_value = MAX_VALUE;

            /* Redraw the instrument */

            if (!b_abort)
            {
               v_draw(
                  x_display, x_window,
                  x_buffer,
                  gc_background,
                  gc_face,
                  gc_scale,
                  gc_needle,
                  x_font,
                  i_window_width,
                  i_window_height);
            }

            break;
         }
      }
   }

   /* Release allocated resources */

   XFreePixmap(x_display, x_buffer);

   XFreeFont(x_display, x_font);

   XFreeGC(x_display, gc_needle);
   XFreeGC(x_display, gc_scale);
   XFreeGC(x_display, gc_face);
   XFreeGC(x_display, gc_background);

   XDestroyWindow(x_display, x_window);

   XCloseDisplay(x_display);

   return EXIT_SUCCESS;
}

