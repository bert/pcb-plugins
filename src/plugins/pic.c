/*!
 * \file pic.c
 *
 * \author Copyright (C) 2014 by Bert Timmerman <bert.timmerman@xs4all.nl>
 *
 * \copyright Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * \brief Plug-in for an IPC-2141A compliant PCB Impedance Calculator
 * (PIC).
 *
 * Compile like this:\n
 * \n
 * gcc -Ipath/to/pcb/src -Ipath/to/pcb -O2 -shared pic.c -o pic.so
 * \n\n
 * The resulting pic.so file should go in $HOME/.pcb/plugins/\n
 *
 * \warning Be very strict in compiling this plug-in against the exact pcb
 * sources you compiled/installed the pcb executable (i.e. src/pcb) with.\n
 *
 * Usage: PIC()\n
 * Invoke the PCB Impedance Calculator.
 *
 * Usage: DMIC(Selected)\n
 * Usage: dmic(selected)\n
 * Let the Differential Microstrip Impedance Calculator calculate
 * the impedance of the selected differential pair of traces on a
 * surface layer.\n
 * The outcome of the calculation is shown in a plain pop-up dialog
 * window.\n
 * If no (valid) argument is passed, no action is carried out.
 *
 * Usage: OSIC(Selected)\n
 * Usage: osic(selected)\n
 * Let the Off-center Stripline Impedance Calculator calculate
 * the impedance of the selected trace in an embedded layer.\n
 * The outcome of the calculation is shown in a plain pop-up dialog
 * window.\n
 * If no (valid) argument is passed, no action is carried out.
 *
 * <hr>
 * This program is free software; you can redistribute it and/or modify\n
 * it under the terms of the GNU General Public License as published by\n
 * the Free Software Foundation; either version 2 of the License, or\n
 * (at your option) any later version.\n
 * \n
 * This program is distributed in the hope that it will be useful,\n
 * but WITHOUT ANY WARRANTY; without even the implied warranty of\n
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.\n
 * \n
 * You should have received a copy of the GNU General Public License\n
 * along with this program; if not, write to:\n
 * the Free Software Foundation, Inc.,\n
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.\n
 */


#include <config.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <math.h>

#include "config.h"
#include "global.h"
#include "data.h"
#include "hid.h"
#include "misc.h"
#include "create.h"
#include "error.h"
#include "rtree.h"
#include "undo.h"
#include "draw.h"
#include "set.h"


#define PIC_VERSION "0.1"

/* Function prototypes. */
static double eifk (double k);
static double eisk (double k);
static double bangbang (double x);
static double sech (double x);
double tanh (double x);
static double coth (double x);

static double Z0 (double w, double t, double b, double er);
static double Z0_zt (double w, double b, double er);
static double Z0_even_zt (double w, double t, double b, double er, double s);
static double Z0_odd_zt (double w, double t, double b, double er, double s);
static double Cfp (double x, double er);
static double Cfp_zt (double er);
static double er_eff (double er, double w, double h);
static double mic (double er, double w, double t, double h);
static double emic (double er, double w, double t, double h1, double h2);
static double ecsic (double er, double w, double s, double t, double b);


/* Following global variables contain "unitless" values. */
double er;
  /*!< Relative Dielectric constant. */
double w;
  /*!< Trace width. */
double t;
  /*!< Trace thickness. */
double s;
  /*!< Edge to edge trace spacing. */
double b;
  /*!< Distance between planes (stripline). */
double h;
  /*!< Distance between trace and plane (microstrip). */
double h1;
  /*!< Distance between trace and upper plane (stripline). */
double h2;
  /*!< Distance between trace and lower plane (stripline). */
double impedance;
  /*!< Calculated impedance. */
char *length_unit;
  /*!< Length unit [mm, mil]. */


/* =================== PCB specific functions =================== */

double
get_width ()
{
  int flag;
  ArcType *arc;
  LineType *line;
  double result;

  result = 0.0;
  /* Find the width of the selected traces (line or arc). */
  flag = FOUNDFLAG;
  ALLLINE_LOOP (PCB->Data);
  {
    if (TEST_FLAG (flag, line))
    {
      w = line->Thickness;
      break;
    }
  }
  ENDALL_LOOP; /* ALLLINE_LOOP */
  ALLARC_LOOP (PCB->Data);
  {
    if (TEST_FLAG (flag, arc))
    {
      w = arc->Thickness;
      break;
    }
  }
  ENDALL_LOOP; /* ALLARC_LOOP */
  if (Settings.grid_unit)
  {
    result = COORD_TO_MIL (w);
  }
  else
  {
    result = COORD_TO_MM (w);
  }
  return result;
}


void
get_length_unit ()
{
  /* Find out what length unit to use. */
  if (Settings.grid_unit)
  {
    /* mils. */
    length_unit = strdup (_("mil"));
  }
  else
  {
    /* mm.*/
    length_unit = strdup (_("mm"));
  }
}


/* =================== Helper math functions =================== */

/*!
 * \brief Calculate the eliptic integral of the first kind.
 */
static double
eifk (double k)
{
  int n;
  int num;
  double denom;
  double result;

  result = 0;
  for (n = 0; n < 10; n++)
  {
    num = bangbang (2 * n - 1);
    denom = bangbang (2 * n);
    result += pow (num / denom, 2) * pow (k, 2 * n);
  }
  result *= M_PI * 2;
  return result;
}


/*!
 * \brief Calculate the eliptic integral of the second kind.
 */
static double
eisk (double k)
{
  int n;
  int num;
  double denom;
  double result;

  result = 0;
  for (n = 1; n < 10; n++)
  {
    num = bangbang (2 * n - 1);
    denom = bangbang (2 * n);
    result += pow (num / denom, 2) * pow (k, 2 * n) / (2 * n - 1);
  }
  result = 1 - result;
  result *= M_PI * 2;
  return result;
}


/*!
 * \brief Calculate the double factorial of \c x.
 */
static double
bangbang (double x)
{
  int i;
  double x2;
  double result;

  result = 1;
  if (x == 0 || x == (-1))
  {
    return (result);
  }
  x2 = x / 2;
  for (i = 0; i < x2; i++)
  {
    result *= x;
    x -= 2;
  }
  return result;
}


/*!
 * \brief Calculate the hyperbolic secant of \c x.
 */
static double
sech (double x)
{
  double result;

  result = 2 / (exp (x) + exp (-1 * x));
  return result;
}


/*!
 * \brief Calculate the hyperbolic tangent of \c x.
 */
double
tanh (double x)
{
  double result;

  result = (exp (x) - exp (-1 * x)) / (exp (x) + exp (-1 * x));
  return result;
}


/*!
 * \brief Calculate the hyperbolic cotangent of \c x.
 */
static double
coth (double x)
{
  double result;

  result = 1 / tanh (x);
  return result;
}


/* =================== impedance components =================== */

/*!
 * \brief Calculate finite-thickness impedance of isolated stripline.
 */
static double
Z0 (double w, double t, double b, double er)
{
  double m;
  double dw1;
  double dw2;
  double dw3;
  double dw;
  double wp;
  double z01;
  double z02;
  double z03;
  double z04;
  double result;

  m = 6 / (3 + (2 * t / (b - t)));
  dw1 = 1 / (2 * (b - t)/t + 1);
  dw2 = (1 / (4 * M_PI)) / (w / t + 1.1);
  dw3 = pow (dw1, 2) + pow (dw2, m);
  dw = (t / M_PI) * ( 1 - 0.5 * log (dw3));
  wp = w + dw;
  z01 = 30 / sqrt (er);
  z02 = (4 * (b-t)) / ( M_PI * wp);
  z03 = z02 * 2;
  z04 = sqrt (pow (z03, 2) + 6.27);
  result = z01 * log (1 + z02 * (z03 + z04));
  return result;
}


/*!
 * \brief Calculate zero-thickness impedance of isolated stripline.
 */
static double
Z0_zt (double w, double b, double er)
{
  double k;
  double kp;
  double result;

  k = sech (M_PI * w / (2 * b));
  kp = tanh (M_PI * w / (2 * b));
  result = 30 * M_PI / sqrt (er);
  result *= eifk (k) / eifk (kp);
  return result;
}


/*!
 * \brief Calculate zero-thickness even impedance of edge-coupled
 * stripline.
 */
static double
Z0_even_zt (double w, double t, double b, double er, double s)
{
  double ke;
  double kep;
  double result;

  ke  = tanh (M_PI * w / (2 * b));
  ke *= tanh (M_PI * (w + s) / (2 * b));
  kep = sqrt (1 - pow (ke, 2));
  result = 30 * M_PI / sqrt (er);
  result *= eifk (kep) / eifk (ke);
  return result;
}


/*!
 * \brief Calculate zero-thickness odd impedance of edge-coupled
 * stripline.
 */
static double
Z0_odd_zt (double w, double t, double b, double er, double s)
{
  double ko;
  double kop;
  double result;

  ko = tanh (M_PI * w / (2 * b));
  ko *= coth (M_PI * (w + s) / (2 * b));
  kop = sqrt (1 - pow (ko, 2));
  result = 30 * M_PI / sqrt (er);
  result *= eifk (kop) / eifk (ko);
  return result;
}


/*!
 * \brief .
 */
static double
Cfp (double x, double er)
{
  double A;
  double result;

  A = 1 / (1 - x);
  result = 0.0885 * er / M_PI;
  result *= (2 * A * log (A + 1)) - ((A - 1) * log (A * A - 1));
  return result;
}


/*!
 * \brief .
 */
static double
Cfp_zt (double er)
{
  double result;

  result = 0.0885 * er / M_PI;
  result *= 2 * log (2);
  return result;
}


/*!
 * \brief Calculate the effective Relative Dielectric constant.
 *
 * When
   \f$ w\h < 1 \f$
 * \n
   \f$
   er_{eff} = \frac {er + 1} {2} + \frac {er - 1} {2} \cdot \left{ \sqrt {\frac {w} {w + 12 h} } + 0.04 \cdot \left( 1 - \frac {w} {h} \right) ^{2} \right}
   \f$ 
 * When
   \f$ w\h \geq 1 \f$
 * \n
   \f$
   er_{eff} = \frac {er + 1} {2} + \frac {er - 1} {2} \cdot \left{ \sqrt {\frac {w} {w + 12 h} }
   \f$ 
 * \n
 */
static double
er_eff
(
  double er,
  /*!< Relative Dielectric constant. */
  double w,
  /*!< Trace width. */
  double h
  /*!< Distance between trace and plane (microstrip). */
)
{
  double result;

  result = 0.0;
  if ((w / h) < 1)
  {
    result = ((er + 1) / 2) + ((er - 1) / 2) * (sqrt (w / (w + 12 * h))
      + 0.04 * pow ((1 - w / h), 2));
  }
  else
  {
    result = ((er + 1) / 2) + ((er - 1) / 2) * (sqrt (w / (w + 12 * h)));
  }
  return result;
}


/*!
 * \brief Calculate the impedance for the specified microstrip trace.
 *
 * <h2>Microstrip.</h2>
 *
 * <h3>Introduction.</h3>
 *
 * The microstrip is a very simple yet useful way to create a
 * transmission line with a PCB.\n
 * There are some advantages to using a microstrip transmission line
 * over other alternatives.\n
 * Modeling approximation can be used to design the microstrip trace.
 * By understanding the microstrip transmission line, designers can
 * properly build these structures to meet their needs.
 *
 * <h3>Description.</h3>
 *
 * A microstrip is constructed with a flat conductor suspended over a
 * ground plane.
 * The conductor and ground plane are seperated by a dielectric.
 * The suface microstrip transmission line also has free space (air) as
 * the dielectric above the conductor.
 * This structure can be built in materials other than printed circuit
 * boards, but will always consist of a conductor separated from a
 * ground plane by some dielectric material.
 *
 * <h3>Microstrip Transmission Line Models.</h3>
 * Models have been created to approximate the characteristics of the
 * microstrip transmission line.\n
 *
 * The characteristic impedance, \f$ Z_{0,surf} \f$ is:

   \f$
     Z_{0,surf} = \frac {\eta_0} {2 \cdot \pi \cdot \sqrt {2} \cdot \sqrt {e_{r,eff} + 1} }
     \cdot \ln \left\{ {1 + 4 \cdot \left( \frac {h} {w_{eff} } \right)
     \cdot \left[ 4 \cdot \left( \frac {14 \cdot er_{eff} + 8} {11 \cdot e_{r,eff} } \right)
     \cdot \left( \frac {h} {w_{eff} \right) }
     + \sqrt {16 \cdot \left( \frac {h} {w_{eff} } \right) ^ {2}
     \cdot  \left( \frac {14 \cdot er_{eff} + 8} {11 \cdot e_{r,eff} } \right) ^ {2}
     + \left( \frac {e_{r,eff} + 1} {2 \cdot e_{r,eff} } \right) \cdot \pi ^ {2} }
     \right] \right\}
   \f$

 * where \f$ \eta_0 \f$ is the wave impedance of free space,
 * \f$ w_{eff} \f$ is the effective signal line width:

   \f$
     w_{eff} = w + \left( \frac {t} {\pi} \right)
     \cdot \ln \left\{ \frac { 4 \cdot e } 
     { \sqrt { { \left( \frac {t} {h} } ^{2} \right) }
     + \left( \frac {t} {w \cdot \pi + 1.1 \cdot t \cdot \pi \right) ^{2} } }
     \right\} \cdot \frac {er_{eff} + 1} {2 \cdot er_{eff} }
   \f$

 * and when \f$ \frac {w} {h} < 1 \f$

   \f$
     \varepsilon_{r,eff} = \frac {\varepsilon_r + 1} {2} + \frac {\varepsilon_r - 1} {2} \cdot
     \left\{ \sqrt { \frac {w} {w + 12 \cdot h} } + 0.04 \cdot
     \left( 1 - \frac {w} {h} \right) ^ {2}
     \right\}
   \f$

 * or when \f$ \frac {w} {h} \geq 1 \f$

   \f$
     \varepsilon_{r,eff} = \frac {\varepsilon_r + 1} {2} + \frac {\varepsilon_r - 1} {2} \cdot
     \sqrt { \frac {w} {w + 12 \cdot h} }
   \f$

 * where \f$ w \f$ is the width of the signal line, \f$ t \f$ is the
 * thickness of the signal line, \f$ h \f$ is the separation between
 * signal line and the reference plane, and \f$ \varepsilon_r \f$ is the
 * relative permittivity of the substrate material.\n
 * The accuracy of these equations is better than \f$ \pm 2 \% \f$.\n
 * For more accuracy, the effect of the conductor thickness should be
 * considered.
 *
 * The source for these formulas are found in the IPC-2141A (2004)
 * “Design Guide for High-Speed Controlled Impedance Circuit Boards”.
 *
 * <h3>Output.</h3>
 *
 * The outcome of the calculation is shown in the pcb log window.\n
 * If no (valid) argument is passed, no action is carried out.\n
 */
static double
mic
(
  double er,
    /*!< Relative Dielectric constant. */
  double w,
    /*!< Trace width. */
  double t,
    /*!< Trace thickness. */
  double h
    /*!< Distance between trace and plane (microstrip). */
)
{
  double w_eff;
  double temp1;
  double temp2;
  double temp3;
  double temp4;
  double result;

  result = 0.0;
  w_eff = w + (t / M_PI) * log (4 * exp (1) / sqrt (pow ((t / h), 2)
    + pow ((t / (w * M_PI + 1.1 * t * M_PI)), 2))) * ((er_eff (er, w, h) + 1) / 2 * er_eff (er, w, h));
  temp1 = sqrt ((16 * pow ((h / w_eff), 2) * pow (((14 * er_eff (er, w, h) + 8) / (11 * er_eff (er, w, h))), 2)
    + ((er_eff (er, w, h) + 1) / (2 * er_eff (er, w, h))) * pow ((M_PI), 2)));
  temp2 = 4 * (h / w_eff) * ((14 * er_eff (er, w, h) + 8) / ( 11 * er_eff (er, w, h)));
  temp3 = 4 * (h / w_eff);
  temp4 = (377 / (2 * M_PI * sqrt (2) * sqrt (er_eff (er, w, h) + 1)));
  result = temp4 * log (1 + temp3 * (temp2 + temp1));
  /* Log input and results in the log window. */
  Message
  (
    _("Microstrip Impedance Calculator (version ") "%s).\n", PIC_VERSION,
    _("Input values:\n"
      "Substrate dielectric (Er) = %d \n"), er,
    _("Trace width (W)           = %d %s\n"), w, Settings.grid_unit,
    _("Trace thickness (T)       = %d %s\n"), t, Settings.grid_unit,
    _("Substrate height (H)      = %d %s\n"), h, Settings.grid_unit,
    _("Results:\n"
      "Impedance (Z)             = %d Ohms\n"), result,
    _("Epsilon (Er_eff)          = %d \n\n"), er_eff
  );
  return result;
}


/*!
 * \brief Calculate the impedance for the specified traces
 * (edge coupled stripline).
 *
 * <h2>Edge Coupled Stripline.</h2>
 *
 * <h3>Introduction.</h3>
 *

 * The source for these formulas are found in the IPC-2141A (2004)
 * “Design Guide for High-Speed Controlled Impedance Circuit Boards”.
 *
 * <h3>Output.</h3>
 * The outcome of the calculation is shown in the pcb log window.\n
 * If no (valid) argument is passed, no action is carried out.\n
 */
static double
ecsic
(
  double er,
    /*!< Substrate Dielectric constant. */
  double w,
    /*!< Trace width. */
  double s,
    /*!< Trace spacing. */
  double t,
    /*!< Trace thickness. */
  double b
    /*!< Substrate height. */
)
{
  double A1;
  double A2;
  double A3_even;
  double A3_odd;
  double A4_even;
  double A4_odd;
  double Z0_even;
  double Z0_odd;
  double result;

  /* Sanity check. */
  if ((s / t) < 5)
  {
    fprintf (stderr, "WARNING: traces too close\n");
  }
  /* Calculate intermediate results. */
  result = 0.0;
  A1 = 1 / Z0 (w, t, b, er);
  A2 = Cfp (t / b, er) / Cfp_zt (er);
  A3_even = 1 / Z0_zt (w, b, er);
  A3_odd = 1 / Z0_odd_zt (w, t, b, er, s);
  A4_even = 1 / Z0_even_zt (w, t, b, er, s);
  A4_odd = 1 / Z0_zt (w, b, er);
  /* Calculate final results. */
  Z0_even = 1 / (A1 - A2 * (A3_even - A4_even));
  Z0_odd = 1 / (A1 + A2 * (A3_odd - A4_odd));
  Z0_odd *= 2;
  /* Round to .1 ohms. */
  result = round (Z0_odd * 10) / 10;
  /* Log input and results in the log window. */
  Message
  (
    _("Edge Coupled Stripline Impedance Calculator (version ") "%s).\n", PIC_VERSION,
    _("Input values:\n"
      "Substrate dielectric (Er) = %d \n"), er,
    _("Trace width (W)           = %d %s\n"), w, Settings.grid_unit,
    _("Trace thickness (T)       = %d %s\n"), t, Settings.grid_unit,
    _("Trace spacing (S)         = %d %s\n"), s, Settings.grid_unit,
    _("Substrate height (B)      = %d %s\n"), b, Settings.grid_unit,
    _("Results:\n"
      "Impedance (Z)             = %d Ohms\n"), result
  );
  return result;
}


/*!
 * \brief Calculate the impedance for the specified embedded microstrip
 * trace.
 *
 * <h2>Embedded Microstrip.</h2>
 *
 * <h3>Introduction.</h3>
 *

 * The source for these formulas are found in the IPC-2141A (2004)
 * “Design Guide for High-Speed Controlled Impedance Circuit Boards”.
 *
 * <h3>Output.</h3>
 *
 * The outcome of the calculation is shown in the pcb log window.\n
 * If no (valid) argument is passed, no action is carried out.\n
 */
static double
emic
(
  double er,
    /*!< Relative Dielectric constant. */
  double w,
    /*!< Trace width. */
  double t,
    /*!< Trace thickness. */
  double h1,
    /*!< Distance between trace and plane (microstrip). */
  double h2
    /*!< Distance between trace and plane (microstrip). */
)
{
  double b;
  double zo_embed;
  double zo_surf;
  double result;

  zo_surf = mic (er, w, t, h1);
  b = h2 - h1;
  zo_embed = zo_surf * (1 / sqrt ((exp ((-2 * b) / h1))
    + (er / er_eff (er, w, h1)) * (1 - exp ((-2 * b) / h1))));
  result = zo_embed;
  /* Log input and results in the log window. */
  Message
  (
    _("Embedded Microstrip Impedance Calculator (version ") "%s).\n", PIC_VERSION,
    _("Input values:\n"
      "Substrate dielectric (Er) = %d \n"), er,
    _("Trace width (W)           = %d %s\n"), w, Settings.grid_unit,
    _("Trace thickness (T)       = %d %s\n"), t, Settings.grid_unit,
    _("Substrate height (H1)     = %d %s\n"), h1, Settings.grid_unit,
    _("Substrate height (H2)     = %d %s\n"), h2, Settings.grid_unit,
    _("Results:\n"
      "Impedance (Z)             = %d Ohms\n"), result,
    _("Epsilon (Er_eff)          = %d \n\n"), er_eff
  );
  return result;
}


/* =================== GTK UI code =================== */

#include <gtk/gtk.h>

/* Function prototypes. */
GtkWidget *lookup_widget (GtkWidget *widget, const gchar *widget_name);
void on_close_button_clicked (GtkButton *button, gpointer user_data);
static void on_destroy (GtkWidget *widget, gpointer data);
void on_calculate_button_clicked (GtkWidget *widget, gpointer user_data);
void on_calculate_mic_button_clicked (GtkWidget *widget, gpointer user_data);
void on_calculate_emic_button_clicked (GtkWidget *widget, gpointer user_data);
void on_distance_B_entry_changed (GtkEditable *editable, gpointer user_data);
void on_distance_H_entry_changed (GtkEditable *editable, gpointer user_data);
void on_distance_H1_entry_changed (GtkEditable *editable, gpointer user_data);
void on_distance_H2_entry_changed (GtkEditable *editable, gpointer user_data);
void on_epsilon_entry_changed (GtkEditable *editable, gpointer user_data);
void on_spacing_entry_changed (GtkEditable *editable, gpointer user_data);
void on_thickness_entry_changed (GtkEditable *editable, gpointer user_data);
void on_width_entry_changed (GtkEditable *editable, gpointer user_data);
static int mic_dialog (int argc, char **argv, Coord x, Coord y);
static int emic_dialog (int argc, char **argv, Coord x, Coord y);
static int ecsic_dialog (int argc, char **argv, Coord x, Coord y);


GtkWidget *
lookup_widget (GtkWidget *widget, const gchar *widget_name)
{
  GtkWidget *parent, *found_widget;

  for (;;)
  {
    if (GTK_IS_MENU (widget))
      parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
    else
      parent = widget->parent;
    if (!parent)
      parent = (GtkWidget*) g_object_get_data (G_OBJECT (widget), "GladeParentKey");
    if (parent == NULL)
      break;
    widget = parent;
  }
  found_widget = (GtkWidget*) g_object_get_data (G_OBJECT (widget), widget_name);
  if (!found_widget)
    g_warning ("Widget not found: %s", widget_name);
  return found_widget;
}


/*!
 * \brief The "Calculate" button is clicked.
 *
 * Calculate the application.
 *
 * <b>Parameters:</b> \c *button is the caller widget.\n
 * \n
 * <b>Parameters:</b> \c user_data.\n
 * \n
 * <b>Returns:</b> none.
 */
void
on_calculate_button_clicked (GtkWidget *widget, gpointer user_data)
{
}


/*!
 * \brief The "Calculate" button in the MIC dialog is clicked.
 *
 * Calculate the application.
 *
 * <b>Parameters:</b> \c *button is the caller widget.\n
 * \n
 * <b>Parameters:</b> \c user_data.\n
 * \n
 * <b>Returns:</b> none.
 */
void
on_calculate_mic_button_clicked (GtkWidget *widget, gpointer user_data)
{
  GtkWidget *impedance_value_label;
  gchar *impedance_string;

  impedance_value_label = NULL;
  impedance_value_label = lookup_widget (GTK_WIDGET (impedance_value_label), "impedance_value_label");
  if (impedance_value_label == NULL)
    return;
  impedance = mic (er, w, t, h);
  impedance_string = g_strdup_printf (" %f ", impedance);
  gtk_label_set_text (GTK_LABEL (impedance_value_label), impedance_string);
}


/*!
 * \brief The "Calculate" button in the EMIC dialog is clicked.
 *
 * Calculate the application.
 *
 * <b>Parameters:</b> \c *button is the caller widget.\n
 * \n
 * <b>Parameters:</b> \c user_data.\n
 * \n
 * <b>Returns:</b> none.
 */
void
on_calculate_emic_button_clicked (GtkWidget *widget, gpointer user_data)
{
  GtkWidget *impedance_value_label;
  gchar *impedance_string;

  impedance_value_label = NULL;
  impedance_value_label = lookup_widget (GTK_WIDGET (impedance_value_label), "impedance_value_label");
  if (impedance_value_label == NULL)
    return;
  impedance = emic (er, w, t, h1, h2);
  impedance_string = g_strdup_printf (" %f ", impedance);
  gtk_label_set_text (GTK_LABEL (impedance_value_label), impedance_string);
}


/*!
 * \brief The "Close" button is clicked.
 *
 * Close the application.
 *
 * <b>Parameters:</b> \c *button is the caller widget.\n
 * \n
 * <b>Parameters:</b> \c user_data.\n
 * \n
 * <b>Returns:</b> none.
 */
void
on_close_button_clicked (GtkButton *button, gpointer user_data)
{
  gtk_main_quit();
}


/*!
 * \brief Terminate the GTK main loop.
 */
static void
on_destroy (GtkWidget *widget, gpointer data)
{
  gtk_main_quit ();
}


/*!
 * \brief The "Distance (B)" entry is changed.
 *
 * <ul>
 * <li>get the chars from the entry.
 * <li>convert to a double and store in the \c b distance variable (global).
 * </ul>
 *
 * <b>Parameters:</b> \c *editable is the caller widget.\n
 * \n
 * <b>Parameters:</b> \c user_data.\n
 * \n
 * <b>Returns:</b> none.
 */
void
on_distance_B_entry_changed (GtkEditable *editable, gpointer user_data)
{
  gchar *leftovers;
  GtkWidget *distance_B_entry;
  const gchar* distance_string;

  distance_B_entry = lookup_widget (GTK_WIDGET (editable), "distance_B_entry");
  distance_string = gtk_entry_get_text (GTK_ENTRY (distance_B_entry));
  b = g_ascii_strtod (distance_string, &leftovers);
}


/*!
 * \brief The "Distance (H)" entry is changed.
 *
 * <ul>
 * <li>get the chars from the entry.
 * <li>convert to a double and store in the \c h distance variable (global).
 * </ul>
 *
 * <b>Parameters:</b> \c *editable is the caller widget.\n
 * \n
 * <b>Parameters:</b> \c user_data.\n
 * \n
 * <b>Returns:</b> none.
 */
void
on_distance_H_entry_changed (GtkEditable *editable, gpointer user_data)
{
  gchar *leftovers;
  GtkWidget *distance_H_entry;
  const gchar* distance_string;

  distance_H_entry = lookup_widget (GTK_WIDGET (editable), "distance_H_entry");
  distance_string = gtk_entry_get_text (GTK_ENTRY (distance_H_entry));
  h = g_ascii_strtod (distance_string, &leftovers);
}


/*!
 * \brief The "Distance (H1)" entry is changed.
 *
 * <ul>
 * <li>get the chars from the entry.
 * <li>convert to a double and store in the \c h1 distance variable (global).
 * </ul>
 *
 * <b>Parameters:</b> \c *editable is the caller widget.\n
 * \n
 * <b>Parameters:</b> \c user_data.\n
 * \n
 * <b>Returns:</b> none.
 */
void
on_distance_H1_entry_changed (GtkEditable *editable, gpointer user_data)
{
  gchar *leftovers;
  GtkWidget *distance_H1_entry;
  const gchar* distance_string;

  distance_H1_entry = lookup_widget (GTK_WIDGET (editable), "distance_H1_entry");
  distance_string = gtk_entry_get_text (GTK_ENTRY (distance_H1_entry));
  h1 = g_ascii_strtod (distance_string, &leftovers);
}


/*!
 * \brief The "Distance (H2)" entry is changed.
 *
 * <ul>
 * <li>get the chars from the entry.
 * <li>convert to a double and store in the \c h2 distance variable (global).
 * </ul>
 *
 * <b>Parameters:</b> \c *editable is the caller widget.\n
 * \n
 * <b>Parameters:</b> \c user_data.\n
 * \n
 * <b>Returns:</b> none.
 */
void
on_distance_H2_entry_changed (GtkEditable *editable, gpointer user_data)
{
  gchar *leftovers;
  GtkWidget *distance_H2_entry;
  const gchar* distance_string;

  distance_H2_entry = lookup_widget (GTK_WIDGET (editable), "distance_H2_entry");
  distance_string = gtk_entry_get_text (GTK_ENTRY (distance_H2_entry));
  h2 = g_ascii_strtod (distance_string, &leftovers);
}


/*!
 * \brief The "Epsilon" entry is changed.
 *
 * <ul>
 * <li>get the chars from the entry.
 * <li>convert to a double and store in the \c er variable (global).
 * </ul>
 *
 * <b>Parameters:</b> \c *editable is the caller widget.\n
 * \n
 * <b>Parameters:</b> \c user_data.\n
 * \n
 * <b>Returns:</b> none.
 */
void
on_epsilon_entry_changed (GtkEditable *editable, gpointer user_data)
{
  gchar *leftovers;
  GtkWidget *epsilon_entry;
  const gchar* epsilon_string;

  epsilon_entry = lookup_widget (GTK_WIDGET (editable), "epsilon_entry");
  epsilon_string = gtk_entry_get_text (GTK_ENTRY (epsilon_entry));
  er = g_ascii_strtod (epsilon_string, &leftovers);
}


/*!
 * \brief The "Spacing (S)" entry is changed.
 *
 * <ul>
 * <li>get the chars from the entry.
 * <li>convert to a double and store in the \c spacing variable (global).
 * </ul>
 *
 * <b>Parameters:</b> \c *editable is the caller widget.\n
 * \n
 * <b>Parameters:</b> \c user_data.\n
 * \n
 * <b>Returns:</b> none.
 */
void
on_spacing_entry_changed (GtkEditable *editable, gpointer user_data)
{
  gchar *leftovers;
  GtkWidget *spacing_entry;
  const gchar* spacing_string;

  spacing_entry = lookup_widget (GTK_WIDGET (editable), "spacing_entry");
  spacing_string = gtk_entry_get_text (GTK_ENTRY (spacing_entry));
  s = g_ascii_strtod (spacing_string, &leftovers);
}


/*!
 * \brief The "Thickness (T)" entry is changed.
 *
 * <ul>
 * <li>get the chars from the entry.
 * <li>convert to a double and store in the \c thickness variable (global).
 * </ul>
 *
 * <b>Parameters:</b> \c *editable is the caller widget.\n
 * \n
 * <b>Parameters:</b> \c user_data.\n
 * \n
 * <b>Returns:</b> none.
 */
void
on_thickness_entry_changed (GtkEditable *editable, gpointer user_data)
{
  gchar *leftovers;
  GtkWidget *thickness_entry;
  const gchar* thickness_string;

  thickness_entry = lookup_widget (GTK_WIDGET (editable), "thickness_entry");
  thickness_string = gtk_entry_get_text (GTK_ENTRY (thickness_entry));
  t = g_ascii_strtod (thickness_string, &leftovers);
}


/*!
 * \brief The "Width (W)" entry is changed.
 *
 * <ul>
 * <li>get the chars from the entry.
 * <li>convert to a double and store in the \c width variable (global).
 * </ul>
 *
 * <b>Parameters:</b> \c *editable is the caller widget.\n
 * \n
 * <b>Parameters:</b> \c user_data.\n
 * \n
 * <b>Returns:</b> none.
 */
void
on_width_entry_changed (GtkEditable *editable, gpointer user_data)
{
  gchar *leftovers;
  GtkWidget *width_entry;
  const gchar* width_string;

  width_entry = lookup_widget (GTK_WIDGET (editable), "width_entry");
  width_string = gtk_entry_get_text (GTK_ENTRY (width_entry));
  w = g_ascii_strtod (width_string, &leftovers);
}


/*!
 * \brief Show a Microstrip Impedance Calculator dialog on the screen.
 *
 * <h3>Usage</h3>
 * MIC()\n
 * Invoke the GTK Microstrip Impedance Calculator to calculate the
 * impedance of the selected trace on a surface layer.\n
 */
static int
mic_dialog (int argc, char **argv, Coord x, Coord y)
{
  GtkWidget *window;
  GtkWidget *table;
  GtkWidget *width_label;
  GtkWidget *thickness_label;
  GtkWidget *distance_H_label;
  GtkWidget *epsilon_label;
  GtkWidget *impedance_label;
  GtkWidget *impedance_value_label;
  GtkWidget *length_unit_label;
  GtkWidget *ohm_label;
  GtkWidget *width_entry;
  GtkWidget *thickness_entry;
  GtkWidget *distance_H_entry;
  GtkWidget *epsilon_entry;
  GtkWidget *calculate_button;
  GtkTooltips *tooltips = NULL;

  /* Initialze GTK. */
  gtk_init (&argc, &argv);
  /* Create a new top level window */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  /* Give the window a 20px wide border */
  gtk_container_set_border_width (GTK_CONTAINER (window), 20);
  /* Give the window the default title */
  gtk_window_set_title (GTK_WINDOW (window), PACKAGE " " VERSION " Impedance Calculator");
  /* Open the window a bit wider so that both the label and title show up */
  gtk_window_resize (GTK_WINDOW (window), 300, 200);

  /* Create the needed labels. */
  width_label = gtk_label_new (_("Trace width (W)"));
  gtk_widget_set_name (width_label, "width_label");
  gtk_widget_show (width_label);

  thickness_label = gtk_label_new (_("Trace thickness (T)"));
  gtk_widget_set_name (thickness_label, "thickness_label");
  gtk_widget_show (thickness_label);

  distance_H_label = gtk_label_new (_("Substrate Height (H)"));
  gtk_widget_set_name (distance_H_label, "distance_H_label");
  gtk_widget_show (distance_H_label);

  epsilon_label = gtk_label_new (_("Substrate Dielectric (Er)"));
  gtk_widget_set_name (epsilon_label, "epsilon_label");
  gtk_widget_show (epsilon_label);

  impedance_label = gtk_label_new (_("Impedance (Z)"));
  gtk_widget_set_name (impedance_label, "impedance_label");
  gtk_widget_show (impedance_label);

  impedance_value_label = gtk_label_new ("...");
  gtk_widget_set_name (impedance_value_label, "impedance_value_label");
  gtk_widget_show (impedance_value_label);

  /* Find out what length unit to use. */
  get_length_unit();
  length_unit_label = gtk_label_new (strdup (length_unit));
  gtk_widget_set_name (length_unit_label, "length_unit_label");
  gtk_widget_show (length_unit_label);

  ohm_label = gtk_label_new (_("Ohm"));
  gtk_widget_set_name (ohm_label, "ohm_label");
  gtk_widget_show (ohm_label);

  /* Create the needed entries. */
  distance_H_entry = gtk_entry_new ();
  gtk_widget_set_name (distance_H_entry, "distance_H_entry");
  gtk_tooltips_set_tip (tooltips, distance_H_entry, _("Type the distance between planes here"), NULL);
  gtk_entry_set_invisible_char (GTK_ENTRY (distance_H_entry), 8226);
  gtk_widget_show (distance_H_entry);

  epsilon_entry = gtk_entry_new ();
  gtk_widget_set_name (epsilon_entry, "epsilon_entry");
  gtk_tooltips_set_tip (tooltips, epsilon_entry, _("Type the epsilon between planes here"), NULL);
  gtk_entry_set_invisible_char (GTK_ENTRY (epsilon_entry), 8226);
  gtk_widget_show (epsilon_entry);

  thickness_entry = gtk_entry_new ();
  gtk_widget_set_name (thickness_entry, "thickness_entry");
  gtk_tooltips_set_tip (tooltips, thickness_entry, _("Type the trace thickness here"), NULL);
  gtk_entry_set_invisible_char (GTK_ENTRY (thickness_entry), 8226);
  gtk_widget_show (thickness_entry);

  width_entry = gtk_entry_new ();
  gtk_widget_set_name (width_entry, "width_entry");
  gtk_tooltips_set_tip (tooltips, width_entry, _("Type the trace width here"), NULL);
  gtk_entry_set_invisible_char (GTK_ENTRY (width_entry), 8226);
  gtk_widget_show (width_entry);

  /* Create a "Calculate" button. */
  calculate_button = gtk_button_new_with_label (_("Calculate"));
  gtk_tooltips_set_tip (tooltips, calculate_button, _("Calculate the result"), NULL);
  gtk_widget_show (calculate_button);

  /* Create a table. */
  table = gtk_table_new (6, 3, FALSE);
  gtk_widget_set_name (table, "table");
  gtk_widget_show (table);

  /* Attach labels to table (first row). */
  gtk_table_attach (GTK_TABLE (table),
    width_label, 0, 1, 0, 1,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (width_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    width_entry, 1, 2, 0, 1,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (width_entry), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    length_unit_label, 2, 3, 0, 1,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (length_unit_label), 0, 0.5);

  /* Attach labels to table (second row). */
  gtk_table_attach (GTK_TABLE (table),
    thickness_label, 0, 1, 1, 2,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (thickness_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    thickness_entry, 1, 2, 1, 2,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (thickness_entry), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    length_unit_label, 2, 3, 1, 2,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (length_unit_label), 0, 0.5);

  /* Attach labels to table (third row). */
  gtk_table_attach (GTK_TABLE (table),
    distance_H_label, 0, 1, 2, 3,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (distance_H_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    distance_H_entry, 1, 2, 2, 3,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (distance_H_entry), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    length_unit_label, 2, 3, 2, 3,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (length_unit_label), 0, 0.5);

  /* Attach labels to table (fourth row). */
  gtk_table_attach (GTK_TABLE (table),
    epsilon_label, 0, 1, 3, 4,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (epsilon_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    epsilon_entry, 1, 2, 3, 4,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (epsilon_entry), 0, 0.5);
  /* No unit label. */

  /* Attach button to table (accross all columns). */
  gtk_table_attach (GTK_TABLE (table),
    calculate_button, 0, 3, 4, 5,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);

  /* Attach the impedance result labels (across bottom row). */
  gtk_table_attach (GTK_TABLE (table),
    impedance_label, 0, 1, 5, 6,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (impedance_value_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    impedance_label, 1, 2, 5, 6,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (impedance_value_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    ohm_label, 2, 3, 5, 6,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (ohm_label), 0, 0.5);

  /* Hook up signals for the callback functions. */
  g_signal_connect (G_OBJECT (window), "destroy",
    G_CALLBACK (on_destroy), NULL);
  g_signal_connect (G_OBJECT (distance_H_entry), "changed",
    G_CALLBACK (on_distance_H_entry_changed), NULL);
  g_signal_connect (G_OBJECT (epsilon_entry), "changed",
    G_CALLBACK (on_epsilon_entry_changed), NULL);
  g_signal_connect (G_OBJECT (thickness_entry), "changed",
    G_CALLBACK (on_thickness_entry_changed), NULL);
  g_signal_connect (G_OBJECT (width_entry), "changed",
    G_CALLBACK (on_width_entry_changed), NULL);
  g_signal_connect (G_OBJECT (calculate_button), "clicked",
    G_CALLBACK (on_calculate_mic_button_clicked), NULL);

  /* And insert the table into the main window */
  gtk_container_add (GTK_CONTAINER (window), table);

  /* Make sure that everything is visible */
  gtk_widget_show_all (window);
  /* Start the GTK main loop */
  gtk_main ();
  return 0;
}


/*!
 * \brief Show an Embedded Microstrip Impedance Calculator dialog on the screen.
 */
static int
emic_dialog (int argc, char **argv, Coord x, Coord y)
{
  GtkWidget *window;
  GtkWidget *table;
  GtkWidget *width_label;
  GtkWidget *thickness_label;
  GtkWidget *distance_H1_label;
  GtkWidget *distance_H2_label;
  GtkWidget *epsilon_label;
  GtkWidget *impedance_label;
  GtkWidget *impedance_value_label;
  GtkWidget *length_unit_label;
  GtkWidget *ohm_label;
  GtkWidget *width_entry;
  GtkWidget *thickness_entry;
  GtkWidget *distance_H1_entry;
  GtkWidget *distance_H2_entry;
  GtkWidget *epsilon_entry;
  GtkWidget *calculate_button;
  GtkTooltips *tooltips = NULL;

  /* Initialze GTK. */
  gtk_init (&argc, &argv);
  /* Create a new top level window */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  /* Give the window a 20px wide border */
  gtk_container_set_border_width (GTK_CONTAINER (window), 20);
  /* Give the window the default title */
  gtk_window_set_title (GTK_WINDOW (window), PACKAGE " " VERSION " Impedance Calculator");
  /* Open the window a bit wider so that both the label and title show up */
  gtk_window_resize (GTK_WINDOW (window), 300, 200);

  /* Create the needed labels. */
  width_label = gtk_label_new (_("Trace width (W)"));
  gtk_widget_set_name (width_label, "width_label");
  gtk_widget_show (width_label);

  thickness_label = gtk_label_new (_("Trace thickness (T)"));
  gtk_widget_set_name (thickness_label, "thickness_label");
  gtk_widget_show (thickness_label);

  distance_H1_label = gtk_label_new (_("Substrate height (H1)"));
  gtk_widget_set_name (distance_H1_label, "distance_H1_label");
  gtk_widget_show (distance_H1_label);

  distance_H2_label = gtk_label_new (_("Substrate height (H2)"));
  gtk_widget_set_name (distance_H2_label, "distance_H2_label");
  gtk_widget_show (distance_H2_label);

  epsilon_label = gtk_label_new (_("Substrate Dielectric (Er)"));
  gtk_widget_set_name (epsilon_label, "epsilon_label");
  gtk_widget_show (epsilon_label);

  impedance_label = gtk_label_new (_("Impedance (Z)"));
  gtk_widget_set_name (impedance_label, "impedance_label");
  gtk_widget_show (impedance_label);

  impedance_value_label = gtk_label_new ("...");
  gtk_widget_set_name (impedance_value_label, "impedance_value_label");
  gtk_widget_show (impedance_value_label);

  /* Find out what length unit to use. */
  get_length_unit();
  length_unit_label = gtk_label_new (strdup (length_unit));
  gtk_widget_set_name (length_unit_label, "length_unit_label");
  gtk_widget_show (length_unit_label);

  ohm_label = gtk_label_new (_("Ohm"));
  gtk_widget_set_name (ohm_label, "ohm_label");
  gtk_widget_show (ohm_label);

  /* Create the needed entries. */
  distance_H1_entry = gtk_entry_new ();
  gtk_widget_set_name (distance_H1_entry, "distance_H1_entry");
  gtk_tooltips_set_tip (tooltips, distance_H1_entry, _("Type the distance H1 between planes here"), NULL);
  gtk_entry_set_invisible_char (GTK_ENTRY (distance_H1_entry), 8226);
  gtk_widget_show (distance_H1_entry);

  distance_H2_entry = gtk_entry_new ();
  gtk_widget_set_name (distance_H2_entry, "distance_H2_entry");
  gtk_tooltips_set_tip (tooltips, distance_H2_entry, _("Type the distance H2 between planes here"), NULL);
  gtk_entry_set_invisible_char (GTK_ENTRY (distance_H2_entry), 8226);
  gtk_widget_show (distance_H2_entry);

  epsilon_entry = gtk_entry_new ();
  gtk_widget_set_name (epsilon_entry, "epsilon_entry");
  gtk_tooltips_set_tip (tooltips, epsilon_entry, _("Type the epsilon between planes here"), NULL);
  gtk_entry_set_invisible_char (GTK_ENTRY (epsilon_entry), 8226);
  gtk_widget_show (epsilon_entry);

  thickness_entry = gtk_entry_new ();
  gtk_widget_set_name (thickness_entry, "thickness_entry");
  gtk_tooltips_set_tip (tooltips, thickness_entry, _("Type the trace thickness here"), NULL);
  gtk_entry_set_invisible_char (GTK_ENTRY (thickness_entry), 8226);
  gtk_widget_show (thickness_entry);

  width_entry = gtk_entry_new ();
  gtk_widget_set_name (width_entry, "width_entry");
  gtk_tooltips_set_tip (tooltips, width_entry, _("Type the trace width here"), NULL);
  gtk_entry_set_invisible_char (GTK_ENTRY (width_entry), 8226);
  gtk_widget_show (width_entry);

  /* Create a "Calculate" button. */
  calculate_button = gtk_button_new_with_label (_("Calculate"));
  gtk_tooltips_set_tip (tooltips, calculate_button, _("Calculate the result"), NULL);
  gtk_widget_show (calculate_button);

  /* Create a table. */
  table = gtk_table_new (6, 3, FALSE);
  gtk_widget_set_name (table, "table");
  gtk_widget_show (table);

  /* Attach labels to table (first row). */
  gtk_table_attach (GTK_TABLE (table),
    width_label, 0, 1, 0, 1,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (width_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    width_entry, 1, 2, 0, 1,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (width_entry), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    length_unit_label, 2, 3, 0, 1,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (length_unit_label), 0, 0.5);

  /* Attach labels to table (second row). */
  gtk_table_attach (GTK_TABLE (table),
    thickness_label, 0, 1, 1, 2,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (thickness_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    thickness_entry, 1, 2, 1, 2,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (thickness_entry), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    length_unit_label, 2, 3, 1, 2,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (length_unit_label), 0, 0.5);

  /* Attach labels to table (third row). */
  gtk_table_attach (GTK_TABLE (table),
    distance_H1_label, 0, 1, 2, 3,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (distance_H1_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    distance_H1_entry, 1, 2, 2, 3,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (distance_H1_entry), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    length_unit_label, 2, 3, 2, 3,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (length_unit_label), 0, 0.5);

  /* Attach labels to table (fourth row). */
  gtk_table_attach (GTK_TABLE (table),
    distance_H2_label, 0, 1, 3, 4,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (distance_H2_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    distance_H2_entry, 1, 2, 3, 4,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (distance_H2_entry), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    length_unit_label, 2, 3, 3, 4,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (length_unit_label), 0, 0.5);

  /* Attach labels to table (fifth row). */
  gtk_table_attach (GTK_TABLE (table),
    epsilon_entry, 1, 2, 4, 5,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (epsilon_entry), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    epsilon_label, 0, 1, 4, 5,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (epsilon_label), 0, 0.5);
  /* No unit label. */

  /* Attach calculate button across all columns (sixth row). */
  gtk_table_attach (GTK_TABLE (table),
    calculate_button, 0, 3, 5, 6,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (calculate_button), 0, 0.5);

  /* Attach the impedance result labels (across bottom row). */
  gtk_table_attach (GTK_TABLE (table),
    impedance_label, 0, 1, 5, 6,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (impedance_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    impedance_label, 1, 2, 5, 6,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (impedance_value_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    ohm_label, 2, 3, 5, 6,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (ohm_label), 0, 0.5);

  /* Hook up signals for the callback functions. */
  g_signal_connect (G_OBJECT (window), "destroy",
    G_CALLBACK (on_destroy), NULL);
  g_signal_connect (G_OBJECT (distance_H1_entry), "changed",
    G_CALLBACK (on_distance_H1_entry_changed), NULL);
  g_signal_connect (G_OBJECT (distance_H2_entry), "changed",
    G_CALLBACK (on_distance_H2_entry_changed), NULL);
  g_signal_connect (G_OBJECT (epsilon_entry), "changed",
    G_CALLBACK (on_epsilon_entry_changed), NULL);
  g_signal_connect (G_OBJECT (thickness_entry), "changed",
    G_CALLBACK (on_thickness_entry_changed), NULL);
  g_signal_connect (G_OBJECT (width_entry), "changed",
    G_CALLBACK (on_width_entry_changed), NULL);
  g_signal_connect (G_OBJECT (calculate_button), "clicked",
    G_CALLBACK (on_calculate_button_clicked), NULL);

  /* And insert the table into the main window */
  gtk_container_add (GTK_CONTAINER (window), table);

  /* Make sure that everything is visible */
  gtk_widget_show_all (window);
  /* Start the GTK main loop */
  gtk_main ();
  return 0;
}


/*!
 * \brief Show an Edge Coupled Stripline Impedance Calculator dialog on the screen.
 */
static int
ecsic_dialog (int argc, char **argv, Coord x, Coord y)
{
  GtkWidget *window;
  GtkWidget *table;
  GtkWidget *width_label;
  GtkWidget *thickness_label;
  GtkWidget *spacing_label;
  GtkWidget *distance_B_label;
  GtkWidget *epsilon_label;
  GtkWidget *impedance_label;
  GtkWidget *impedance_value_label;
  GtkWidget *length_unit_label;
  GtkWidget *ohm_label;
  GtkWidget *width_entry;
  GtkWidget *thickness_entry;
  GtkWidget *spacing_entry;
  GtkWidget *distance_B_entry;
  GtkWidget *epsilon_entry;
  GtkWidget *calculate_button;
  GtkTooltips *tooltips = NULL;

  /* Initialze GTK. */
  gtk_init (&argc, &argv);
  /* Create a new top level window */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  /* Give the window a 20px wide border */
  gtk_container_set_border_width (GTK_CONTAINER (window), 20);
  /* Give the window the default title */
  gtk_window_set_title (GTK_WINDOW (window), PACKAGE " " VERSION " Impedance Calculator");
  /* Open the window a bit wider so that both the label and title show up */
  gtk_window_set_default_size (GTK_WINDOW (window), 300, 200);

  /* Create the needed labels. */
  width_label = gtk_label_new (_("Trace width (W)"));
  gtk_widget_set_name (width_label, "width_label");
  gtk_widget_show (width_label);

  thickness_label = gtk_label_new (_("Trace thickness (T)"));
  gtk_widget_set_name (thickness_label, "thickness_label");
  gtk_widget_show (thickness_label);

  spacing_label = gtk_label_new (_("Trace spacing (S)"));
  gtk_widget_set_name (spacing_label, "spacing_label");
  gtk_widget_show (spacing_label);

  distance_B_label = gtk_label_new (_("Substrate height (B)"));
  gtk_widget_set_name (distance_B_label, "distance_B_label");
  gtk_widget_show (distance_B_label);

  epsilon_label = gtk_label_new (_("Substrate Dielectric (Er)"));
  gtk_widget_set_name (epsilon_label, "epsilon_label");
  gtk_widget_show (epsilon_label);

  impedance_label = gtk_label_new (_("Impedance (Z)"));
  gtk_widget_set_name (impedance_label, "impedance_label");
  gtk_widget_show (impedance_label);

  impedance_value_label = gtk_label_new ("...");
  gtk_widget_set_name (impedance_value_label, "impedance_value_label");
  gtk_widget_show (impedance_value_label);

  /* Find out what length unit to use. */
  get_length_unit();
  length_unit_label = gtk_label_new (strdup (length_unit));
  gtk_widget_set_name (length_unit_label, "length_unit_label");
  gtk_widget_show (length_unit_label);

  ohm_label = gtk_label_new (_("Ohm"));
  gtk_widget_set_name (ohm_label, "ohm_label");
  gtk_widget_show (ohm_label);

  /* Create the needed entries. */
  distance_B_entry = gtk_entry_new ();
  gtk_widget_set_name (distance_B_entry, "distance_B_entry");
  gtk_tooltips_set_tip (tooltips, distance_B_entry, _("Type the distance between planes here"), NULL);
  gtk_entry_set_invisible_char (GTK_ENTRY (distance_B_entry), 8226);
  gtk_widget_show (distance_B_entry);

  epsilon_entry = gtk_entry_new ();
  gtk_widget_set_name (epsilon_entry, "epsilon_entry");
  gtk_tooltips_set_tip (tooltips, epsilon_entry, _("Type the epsilon between planes here"), NULL);
  gtk_entry_set_invisible_char (GTK_ENTRY (epsilon_entry), 8226);
  gtk_widget_show (epsilon_entry);

  spacing_entry = gtk_entry_new ();
  gtk_widget_set_name (spacing_entry, "spacing_entry");
  gtk_tooltips_set_tip (tooltips, spacing_entry, _("Type the spacing between traces here"), NULL);
  gtk_entry_set_invisible_char (GTK_ENTRY (spacing_entry), 8226);
  gtk_widget_show (spacing_entry);

  thickness_entry = gtk_entry_new ();
  gtk_widget_set_name (thickness_entry, "thickness_entry");
  gtk_tooltips_set_tip (tooltips, thickness_entry, _("Type the trace thickness here"), NULL);
  gtk_entry_set_invisible_char (GTK_ENTRY (thickness_entry), 8226);
  gtk_widget_show (thickness_entry);

  width_entry = gtk_entry_new ();
  gtk_widget_set_name (width_entry, "width_entry");
  gtk_tooltips_set_tip (tooltips, width_entry, _("Type the trace width here"), NULL);
  gtk_entry_set_invisible_char (GTK_ENTRY (width_entry), 8226);
  gtk_widget_show (width_entry);

  /* Create a "Calculate" button. */
  calculate_button = gtk_button_new_with_label (_("Calculate"));
  gtk_tooltips_set_tip (tooltips, calculate_button, _("Calculate the result"), NULL);
  gtk_widget_show (calculate_button);

  /* Create a table */
  table = gtk_table_new (7, 3, FALSE);
  gtk_widget_set_name (table, "table");
  gtk_widget_show (table);

  /* Attach labels to table (left column). */
  gtk_table_attach (GTK_TABLE (table),
    width_label, 0, 1, 0, 1,
    (GtkAttachOptions) (GTK_FILL),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (width_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    thickness_label, 0, 1, 1, 2,
    (GtkAttachOptions) (GTK_FILL),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (thickness_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    spacing_label, 0, 1, 2, 3,
    (GtkAttachOptions) (GTK_FILL),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (spacing_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    distance_B_label, 0, 1, 3, 4,
    (GtkAttachOptions) (GTK_FILL),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (distance_B_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    epsilon_label, 0, 1, 4, 5,
    (GtkAttachOptions) (GTK_FILL),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (epsilon_label), 0, 0.5);

  /* Attach entries to table (middle column). */
  gtk_table_attach (GTK_TABLE (table),
    width_entry, 1, 2, 0, 1,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (width_entry), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    thickness_entry, 1, 2, 1, 2,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (thickness_entry), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    spacing_entry, 1, 2, 2, 3,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (spacing_entry), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    distance_B_entry, 1, 2, 3, 4,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (distance_B_entry), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    epsilon_entry, 1, 2, 4, 5,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (epsilon_entry), 0, 0.5);

  /* Attach units labels to table (right column). */
  gtk_table_attach (GTK_TABLE (table),
    length_unit_label, 2, 3, 0, 1,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (length_unit_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    length_unit_label, 2, 3, 1, 2,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (length_unit_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    length_unit_label, 2, 3, 2, 3,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (length_unit_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    length_unit_label, 2, 3, 3, 4,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (length_unit_label), 0, 0.5);

  /* Attach button to table (accross all columns). */
  gtk_table_attach (GTK_TABLE (table),
    calculate_button, 0, 3, 5 , 6,
    (GtkAttachOptions) (GTK_FILL),
    (GtkAttachOptions) (0), 0, 0);

  /* Attach the impedance result labels (across bottom row). */
  gtk_table_attach (GTK_TABLE (table),
    impedance_label, 0, 1, 6, 7,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (impedance_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    impedance_value_label, 1, 2, 6, 7,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (impedance_value_label), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table),
    ohm_label, 2, 3, 6, 7,
    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (ohm_label), 0, 0.5);

  /* Hook up signals for the callback functions. */
  g_signal_connect (G_OBJECT (window), "destroy",
    G_CALLBACK (on_destroy), NULL);
  g_signal_connect ((gpointer) distance_B_entry, "changed",
    G_CALLBACK (on_distance_B_entry_changed), NULL);
  g_signal_connect ((gpointer) epsilon_entry, "changed",
    G_CALLBACK (on_epsilon_entry_changed), NULL);
  g_signal_connect ((gpointer) spacing_entry, "changed",
    G_CALLBACK (on_spacing_entry_changed), NULL);
  g_signal_connect ((gpointer) thickness_entry, "changed",
    G_CALLBACK (on_thickness_entry_changed), NULL);
  g_signal_connect ((gpointer) width_entry, "changed",
    G_CALLBACK (on_width_entry_changed), NULL);
  g_signal_connect (G_OBJECT (calculate_button), "clicked",
    G_CALLBACK (on_calculate_button_clicked), NULL);

  /* And insert the table into the main window */
  gtk_container_add (GTK_CONTAINER (window), table);

  /* Make sure that everything is visible */
  gtk_widget_show_all (window);
  /* Start the GTK main loop */
  gtk_main ();
  return 0;
}


/* =================== plugin smart stuff =================== */

static HID_Action impedance_action_list[] =
{
  {"mic", NULL, mic_dialog, "Microstrip Impedance Calculator", NULL},
  {"emic", NULL, emic_dialog, "Embedded Microstrip Impedance Calculator", NULL},
  {"ecsic", NULL, ecsic_dialog, "Edge Coupled Stripline Impedance Calculator", NULL},
  {"osic", NULL, ecsic_dialog, "Off-Center Stripline Impedance Calculator", NULL}
};


REGISTER_ACTIONS (impedance_action_list)


void
pcb_plugin_init ()
{
  register_impedance_action_list ();
}


/* EOF */
