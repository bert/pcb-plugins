/* 
 Copyright (C) 2008 Endre Bak <endrebak@yahoo.com>

 Licensed under the terms of the GNU General Public License, version
 2 or later.

*/

package ratsel;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class Connection {
	
	private static final float PIN_PENALTY_RATIO = 1.2f;
	
	private ArrayList<Rat> iRats;
	
	private Pin[] iPins;
	
	private double iLength;
	
	private Key iKey;
	
	public static class Key implements Comparable<Key> {
		
		private String[] iPinNames;
		
		public Key(String[] pPinNames) {
			if (pPinNames == null || pPinNames.length != 2)
				throw new IllegalArgumentException("Length of pPinNamse must be 2!");
			iPinNames = pPinNames;
			// make it pinName order independent
			Arrays.sort(iPinNames);
		}

		public int compareTo(Key pThat) {
			int cmp = iPinNames[0].compareTo(pThat.iPinNames[0]);
			return cmp == 0 ? iPinNames[1].compareTo(pThat.iPinNames[1]) : cmp;
		}
		
		public String toString() {
			return iPinNames[0] + ", " + iPinNames[1];
		}
	}
	
	public Connection(Pin[] pPins, ArrayList<Rat> pRats) {
		iPins = pPins; iRats = pRats;
		calcLength();
		iKey = new Key(new String[] { iPins[0].getName(), iPins[1].getName() });
	}
	
	public Key getKey() { return iKey; }
	
	public Pin[] getPins() { return iPins; }
	
	public List<Rat> getRats() { return iRats; }
	
	public double getLength() { return iLength; }
	
	public String toString() {
		return "Pins: "+ iKey.toString() + ", Rats:"+iRats.size();
	}
	
	private void calcLength() {
		for(Rat rat: iRats) {
			Location l0 = rat.getLocation(0);
			Location l1 = rat.getLocation(1);
			int dx = l0.getX() - l1.getX();
			int dy = l0.getY() - l1.getY();
			iLength = Math.sqrt(dx*dx + dy*dy);
		}
		int mul = iRats.size() - 1;
		if (mul > 0) iLength *= (mul * PIN_PENALTY_RATIO);
	}
	
}