/* 
 Copyright (C) 2008 Endre Bak <endrebak@yahoo.com>

 Licensed under the terms of the GNU General Public License, version
 2 or later.

*/

package ratsel;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;


public class JIFace {
	
	public static native void msg(String s);

	public static native int getNetNameN();

	public static native String getNetName(int pIdx);
	
	public static native int getPinNameN(int pNetNameIdx);
	
	public static native String getPinName(int pNetNameIdx, int pPinIdx);
	
	public static native Pin[] getPins();
	
	public static native Rat[] getRats();
	
	public static native void selectRat(int pIdx);
	
	public static void trc(String pMsg) {
		System.out.print(pMsg);
		msg(pMsg);
	}

	public static int ratSel(String[] pArgs, int pX , int pY) {
		trc("arguments:\n");
		for(String arg: pArgs) trc(" -> "+arg+"\n");
		if (pArgs.length != 1) {
			trc("Error: FileName needed! usage: RatSel(fileName)\n");
			return 1;
		}
		try {
			PCB.importLayout();
			PCB.ratSel(pArgs[0]);
		} catch (Exception e) {
			trc(toString(e));
		}
		return 0;
	}

	public static void main(String[] args) {
		System.out.println("java main");	
	}
	
	private static String toString(Throwable pThr) {
		ByteArrayOutputStream os = new ByteArrayOutputStream();
		pThr.printStackTrace(new PrintStream(os));
		return new String(os.toByteArray());
	}

}
