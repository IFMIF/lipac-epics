/******************************************************************************
 * CEA - Direction des Sciences de la Matiere - IRFU/SIS
 * CE-SACLAY, 91191 Gif-sur-Yvette, France
 *
 * $Id: icv150.h 23 2013-03-13 15:38:34Z lussi $
 *
 * who       when       what
 * --------  --------   ----------------------------------------------
 * ylussign  10/10/07   created
 * ylussign  03/07/08   changed icv150 functions type to void
 */

int devIcv150Verbose;

void icv150CfgScan (int card, int signal);
void icv150CfgGain (int card, int signal, int gain);
void icv150StoreGains (int card);
void icv150CfgExtTrig (int card, int event);
void icv150CfgAutoScan (int card);
void icv150SoftTrig (int card);
void icv150State (int card);
