/* 
 Copyright (C) 2008 Endre Bak <endrebak@yahoo.com>

 Licensed under the terms of the GNU General Public License, version
 2 or later.

*/

package ratsel;


public class Rat {
	
	private Location[] iLocs;
	
	private Pin[] iPins;
	
	private int iId;
	
	public Rat(Location[] pLocs, int pId) {
		iLocs = pLocs;
		iPins = new Pin[] { null, null };
		iId = pId;
	}
	
	public Rat(Location pLoc0, Location pLoc1, int pId) {
		this(new Location[] { pLoc0, pLoc1 }, pId);
	}
	
	public Rat(int pX0, int pY0, int pSideMask0, int pX1, int pY1, int pSideMask1, int pId) {
		this(
			new Location[] {
					new Location(pX0, pY0, pSideMask0), new Location(pX1, pY1, pSideMask1)	
			}, pId
		);
	}
	
	public Location getLocation(int pIdx) { return iLocs[pIdx]; }
	
	public Pin getPin(int pIdx) { return iPins[pIdx]; }
	
	public Pin[] getPins() { return iPins; }
	
	public void setPin(int pIdx, Pin pPin) { iPins[pIdx] = pPin; }
	
	public int getId() { return iId; }
	
	public String toString() {
		return "Rat("+getLoc(0)+", "+getLoc(1)+")";
	}
	
	private String getLoc(int pIdx) {
		return iPins[pIdx] != null ? iPins[pIdx].toString() : iLocs[pIdx].toString();
	}
	
}