/* 
 Copyright (C) 2008 Endre Bak <endrebak@yahoo.com>

 Licensed under the terms of the GNU General Public License, version
 2 or later.

*/


package ratsel;

import java.util.Comparator;

public class Side  {
	
	public static final int SOLDER = 1;
	
	public static final int COMPONENT = 2;
	
	public static final int BOTH = SOLDER|COMPONENT;
	
	private final String[] NAMES = { "none", "solder", "component", "both" };
	
	public static int conCompare(Side pSide0, Side pSide1) {
		if (pSide0.connects(BOTH) || pSide0.connects(BOTH)) return 0;
		return pSide0.getSideMask() - pSide1.getSideMask();
	}
	
	public static final Comparator<Side> CON_COMP = new Comparator<Side>() {
		public int compare(Side pSide0, Side pSide1) {
			return conCompare(pSide0, pSide1);
		}
	};
	
	private int iMask;
	
	public Side(int pMask) { iMask = pMask; }
	
	public int getSideMask() { return iMask; }

	public boolean connects(int pFlag) {
		return (iMask & pFlag) > 0;
	}
	
	public boolean connects(Side pThat) {
		return connects(pThat.iMask);
	}
	
	public String toString() {
		return NAMES[iMask];
	}

	
}