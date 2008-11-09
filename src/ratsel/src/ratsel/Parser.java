/* 
 Copyright (C) 2008 Endre Bak <endrebak@yahoo.com>

 Licensed under the terms of the GNU General Public License, version
 2 or later.

*/

package ratsel;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.SortedSet;
import java.util.TreeSet;

/*
	R1-2 ,  R2-1, R3-(1), Opa2-(2,3) 
	Opa3-(1-5), IC3-( 1, 4,6 -10, 12 ), R20-1
	
	pinDefList	=	pinDef *( ( "," / "\n" ) pinDef )
	pinDef		= 	compName "-" pinNumDef
	pinNumDef	=	pinNum / pinNumGrp
	pinNumGrp	=	"(" grpEntry *( "," grpEntry) ")"
	grpEntry	=	pinNum / pinRange
	pinRange	=	pinNum "-" pinNum
	
	Out-1, normalized: Out-(1-1)
*/

public class Parser {
	
	private static SortedSet<String> cPinSet;
	
	private static String cStr;
	
	private static int cPos, cLinePos, cLineNum;
	
	public static SortedSet<String> parseFile(String pFileName)
	throws FileNotFoundException, IOException {
		InputStreamReader reader = new InputStreamReader(new FileInputStream(pFileName));
		StringBuilder bldr = new StringBuilder();
		char[] buf = new char[4096];
		int read;
		while ((read = reader.read(buf, 0, buf.length)) >= 0) {
			bldr.append(buf, 0, read);
		}
		return parse(bldr.toString());
	}

	public static SortedSet<String> parse(String pStr) {
		if (pStr == null) return null;
		cPinSet = new TreeSet<String>();
		cStr = pStr;
		cPos = cLinePos = cLineNum = 0;
		try {
			readPinDefList();
		} catch (IllegalArgumentException e) {
			throw new IllegalArgumentException(
				"Problem at line:" + cLineNum + ", pos:" + cLinePos + " -> "+e.getMessage(),
				e
			);
		}
		return cPinSet;
	}
	
	private static final int EOF = -1;
	
	private static int getChar() {
		if (cPos >= cStr.length()) { ++cPos; return EOF; }
		int ch = cStr.charAt(cPos);
		++cPos;
		if (ch == '\n') {
			++cLineNum; cLinePos = 0;
		} else {
			++cLinePos;
		}
		return ch;
	}
	
	private static void ungetChar() {
		if (cPos <= 0) return;
		if (--cPos >= cStr.length()) return;
		if (cStr.charAt(cPos) == '\n') {
			cLinePos = 0; --cLineNum;
		} else {
			--cLinePos;
		}
	}
	
	private static boolean isAlpha(int pCh) {
		return (pCh >= 'a' && pCh <= 'z') || (pCh >= 'A' && pCh <= 'Z');
	}

	private static boolean isNumeric(int pCh) {
		return pCh >= '0' && pCh <= '9';
	}

	private static boolean isAlphaNumeric(int pCh) {
		return isNumeric(pCh) || isAlpha(pCh);
	}

	private static boolean isInSet(int pCh, String pSet) {
		return pSet.indexOf(pCh) >= 0;
	}

	private static final String WS = " \t\n";

	private static int getCharSkip(String pToSkip) {
		int ch;
		do {
			ch = getChar();
		} while (isInSet(ch, pToSkip));
		return ch;
	}

	private static boolean skip(String pChrs, String pWS) {
		int ch = getCharSkip(pWS);
		boolean res = isInSet(ch, pChrs);
		if (!res) ungetChar();
		return res;
	}

	private static boolean skipChar(int ch2skip) {
		int ch = getCharSkip(WS);
		boolean res = ch2skip == ch;
		if (!res) ungetChar();
		return res;
	}

	/*
		readPinNum:
			readNumerics
	 */
	private static int readPinNum() {
		StringBuilder bldr = new StringBuilder();
		int ch = getCharSkip(WS);
		if (!isNumeric(ch)) throw new IllegalArgumentException("Number expected!");
		bldr.append((char)ch);
		while (true) {
			ch = getChar();
			if (!isNumeric(ch)) { ungetChar(); break; }
			bldr.append((char)ch);
		}
		return Integer.parseInt(bldr.toString());
	}

	/*
		readGrpEntry:
			readPinNum(); if ('-') readPinNum();
		can write the result array
	 */
	private static void readGrpEntry(String pCompName) {
		int startPin = readPinNum();
		trc("startPin:"+startPin);
		if (skipChar('-')) {
			int endPin = readPinNum();
			trc("endPin:"+endPin);
			int inc = endPin > startPin ? 1 : -1;
			while (true) {
				cPinSet.add(pCompName+"-"+startPin);
				if (startPin == endPin) return;
				startPin += inc;
			}
		} else {
			cPinSet.add(pCompName+"-"+startPin);
		}
	}


	/*
		readPinNumGrp:
			do { readGrpEntry } while (read ',');
	 */
	private static void readPinNumGrp(String pCompName) {
		if (!skipChar('(')) throw new IllegalArgumentException("'(' expected!");
		do {
			readGrpEntry(pCompName);
		} while (skipChar(','));
		if (!skipChar(')')) throw new IllegalArgumentException( "')' expected!");
	}

	private static final String UNEXPECTED_END = "Unexpected end of file!";

	/*
		readPinNumDef:
			if '(' -> readPinNumGrp else -> readPinNum
		can write the result array
	 */
	private static void readPinNumDef(String pCompName) {
		trc("compName:"+pCompName);
		int ch = getCharSkip(WS);
		if (ch == EOF) throw new IllegalArgumentException(UNEXPECTED_END);
		ungetChar();
		if (ch == '(') {
			readPinNumGrp(pCompName);
		} else {
			int pinNum = readPinNum();
			cPinSet.add(pCompName+"-"+pinNum);
		}
	}

	private static final String PINNAME_EXPECTED = "Pin name expected!";

	/*
		readPinDef:
			readTill '-' readPinNumDef
	 */
	private static void readPinDef() {
		StringBuilder bldr = new StringBuilder();
		int ch = getCharSkip(WS);
		if (ch == EOF) return;
		if (!isAlpha(ch)) throw new IllegalArgumentException(PINNAME_EXPECTED);
		bldr.append((char)ch);
		while (true) {
			ch = getChar();
			if (ch == EOF) throw new IllegalArgumentException(UNEXPECTED_END);
			if (!isAlphaNumeric(ch) && !isInSet(ch, ".-_"))
				throw new IllegalArgumentException(PINNAME_EXPECTED);
			if (ch == '-') {
				break; 
			} else {
				bldr.append((char)ch);
			}	
		}
		readPinNumDef(bldr.toString());
	}

	/*
		readPinDefList:
			readPinDef *( read(',' / 'n') ) readPinDef
	 */
	private static void readPinDefList() {
		do {
			readPinDef();
		} while (skip(",\n", " \t"));
	}
	
	private static void trc(String pMsg) {
		//System.out.println("TRC:"+pMsg);
	}
	
}
