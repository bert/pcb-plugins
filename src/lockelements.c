/*!
 * \file lockelements.c
 * \author Copyright (C) 2008 by Bert Timmerman <bert.timmerman@xs4all.nl>
 * \brief Unlocking/locking elements plug-in for PCB.
 *
 * Function to lock/unlock all/selected PCB elements.
 * \n
 * Compile like this:\n
 * \n
 * gcc -Ipath/to/pcb/src -Ipath/to/pcb -O2 -shared lockelements.c -o lockelements.so
 * \n\n
 * The resulting lockelements.so file should go in $HOME/.pcb/plugins/\n
 * \n
 * \warning Be very strict in compiling this plug-in against the exact pcb
 * sources you compiled/installed the pcb executable (i.e. src/pcb) with.\n
 * \n
 * Usage: LockElements([Selected|All])\n
 * Usage: UnlockElements([Selected|All])\n
 * \n
 * If no argument is passed, no locking/unlocking of elements is carried out.\n
 * \n
 * \bug When locking a selection of elements, it is not easy to unselect the
 * selection since those elements and their pins/pads/elementlines/elementarcs
 * are now locked ;)\n
 * This appears to be a bug in pcb.\n
 * \todo A possible workaround would be to clear the selected flag of the
 * locked element at the same instance the lock flag was set.
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


#include <stdio.h>
#include <math.h>

#include "global.h"
#include "data.h"
#include "hid.h"
#include "misc.h"
#include "create.h"
#include "rtree.h"
#include "undo.h"
#include "set.h"

/*!
 * \brief Locking all or selected elements.
 *
 * Usage: LockElements([Selected|All])\n
 * If no argument is passed, no action is carried out.
 */
static int
lock_elements(int argc, char **argv)
{
        int selected;
        int all;
        if (argc > 0 && strcasecmp (argv[0], "Selected") == 0)
                selected = 1;
        else if (argc >0 && strcasecmp (argv[0], "All") == 0)
                all = 1;
        else
        {
                Message ("ERROR: in LockElements argument should be either Selected or All.\n");
                return 1;
        }
        SET_FLAG (NAMEONPCBFLAG, PCB);
        ELEMENT_LOOP(PCB->Data);
        {
                if (!TEST_FLAG (LOCKFLAG, element))
                {
                        /* element is not locked */
                        if (all)
                                SET_FLAG(LOCKFLAG, element);
                        if (selected)
                        {
                                if (TEST_FLAG (SELECTEDFLAG, element))
                                {
                                        /* better to unselect element first */
                                        CLEAR_FLAG(SELECTEDFLAG, element);
                                        SET_FLAG(LOCKFLAG, element);
                                }
                        }
                }
        }
        END_LOOP;
        gui->invalidate_all ();
        IncrementUndoSerialNumber ();
        return 0;
}


/*!
 * \brief Locking all or selected elements.
 *
 * Usage: UnlockElements([Selected|All])\n
 * If no argument is passed, no action is carried out.
 */
static int
unlock_elements(int argc, char **argv)
{
        int selected;
        int all;
        if (argc > 0 && strcasecmp (argv[0], "Selected") == 0)
                selected = 1;
        else if (strcasecmp (argv[0], "All") == 0)
                all = 1;
        else
        {
                Message ("ERROR: in UnlockElements argument should be either Selected or All.\n");
                return 1;
        }
        SET_FLAG (NAMEONPCBFLAG, PCB);
        ELEMENT_LOOP(PCB->Data);
        {
                if (TEST_FLAG (LOCKFLAG, element))
                {
                        /* element is locked */
                        if (all)
                                CLEAR_FLAG(LOCKFLAG, element);
                        if (selected)
                        {
                                if (TEST_FLAG (SELECTEDFLAG, element))
                                        CLEAR_FLAG(LOCKFLAG, element);
                        }
                }
        }
        END_LOOP;
        gui->invalidate_all ();
        IncrementUndoSerialNumber ();
        return 0;
}


static HID_Action lockelements_action_list[] =
{
        {"LockElements", NULL, lock_elements, "Lock selected or all elements", NULL},
        {"UnlockElements", NULL, unlock_elements, "Unlock selected or all elements", NULL}
};


REGISTER_ACTIONS (lockelements_action_list)


void
pcb_plugin_init()
{
        register_lockelements_action_list();
}

/* EOF */
