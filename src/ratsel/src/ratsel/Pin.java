/* 
 Copyright (C) 2008 Endre Bak <endrebak@yahoo.com>

 Licensed under the terms of the GNU General Public License, version
 2 or later.

*/

package ratsel;

import java.util.ArrayList;

public class Pin extends Location {
	
	private String iName;
	private String iElementName;
	private int iNum;
	private int iSeq;
	private ArrayList<Rat> iRats = new ArrayList<Rat>();
	private boolean iTraversed;
	
	public Pin(String pElementName, int pNum, int pSeq, int pX, int pY, Side pSide) {
		super(pX, pY, pSide);
		iElementName = pElementName;
		iNum = pNum;
		iSeq = pSeq;
		iName = pElementName+"-"+pNum;
		iTraversed = false;
	}
	
	public Pin(String pElementName, int pNum, int pSeq, int pX, int pY, int pSideMask) {
		this(pElementName, pNum, pSeq, pX, pY, new Side(pSideMask));
	}
	
	public Pin(String pName, int pSeq, int pX, int pY, Side pSide) {
		super(pX, pY, pSide);
		iName = pName; iSeq = pSeq;
		explodeName();
	}
	
	public Pin(String pName, int pSeq, int pX, int pY, int pSideMask) {
		this(pName, pSeq, pX, pY, new Side(pSideMask));
	}
	
	public String getName() { return iName; }
	
	public String getElementName() { return iElementName; }
	
	public int getNum() { return iNum; }
	
	/**
	 * The seq property is introduced to support PCB style pads, which are represented as lines.
	 * They have to coordinates where rats can connect to.
	 * Here in the plugin a pad is represented by 2 pins with the same name but different seq
	 * values (0 and 1).
	 * @return
	 */
	public int getSeq() { return iSeq; }
	
	public boolean isTraversed() { return iTraversed; }
	
	public void setTraversed(boolean pVal) { iTraversed = pVal; }
	
	public String toString() {
		return "Pin("+iName+", "+super.toString()+")";
	}
	
	public boolean equals(Pin pThat) {
		if (pThat == null) return false;
		return iName.equals(pThat.iName);
	}
	
	public void addRat(Rat pRat) {
		iRats.add(pRat);
	}
	
	public ArrayList<Rat> getRats() {
		return iRats;
	}
	
	private void explodeName() {
		int idx = iName.indexOf('-');
		if (idx < 0) throw new IllegalArgumentException(
			"pinName:"+iName+" doesn't contain the '-' separator character!"
		);
		if (idx == iName.length()-1) throw new IllegalArgumentException(
			"In pinName:"+iName+" pinNumber is expected after the '-' character!"
		);
		iElementName = iName.substring(0, idx);
		iNum = Integer.parseInt(iName.substring(idx+1));
	}
	
}