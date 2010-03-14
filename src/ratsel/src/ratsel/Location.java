/* 
 Copyright (C) 2008 Endre Bak <endrebak@yahoo.com>

 Licensed under the terms of the GNU General Public License, version
 2 or later.

*/

package ratsel;

public class Location implements Comparable<Location> {

	private int iX, iY;
	
	private Side iSide;
	
	public Location(int pX, int pY, Side pSide) {
		iX = pX; iY = pY; iSide = pSide;
	}
	
	public Location(int pX, int pY, int pSideMask) {
		this(pX, pY, new Side(pSideMask));
	}
	
	public int getX() { return iX; }
	
	public int getY() { return iY; }
	
	public Side getSide() { return iSide; }
	
	public boolean connects(Location pThat) {
		if (iX != pThat.iX || iY != pThat.iY) return false;
		if (iSide.connects(pThat.iSide)) return true;
		return iSide == pThat.iSide;
	}
	
	public int compareTo(Location pThat) {
		int res = iX - pThat.iX;
		if (res != 0) return res;
		res = iY - pThat.iY;
		if (res != 0) return res;
		return Side.conCompare(iSide, pThat.iSide);
	}

	public int getSideMask() {
		return iSide.getSideMask();
	}
	
	public String toString() {
		return "("+iX+", "+iY+", "+iSide+")";
	}
	
}