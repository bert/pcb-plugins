/* 
 Copyright (C) 2008 Endre Bak <endrebak@yahoo.com>

 Licensed under the terms of the GNU General Public License, version
 2 or later.

*/

#include "cppIFace.hpp"
#include <gcj/cni.h>
#include <java/lang/Throwable.h>
#include <java/lang/String.h>
#include "ratsel/JIFace.h"
#include "ratsel/Pin.h"
#include "ratsel/Rat.h"

extern "C" {
#include "cIFace.h"
}

#ifndef NULL
#define NULL 0
#endif


using java::lang::String;
using java::lang::Throwable;

using ratsel::JIFace;
using ratsel::Pin;
using ratsel::Rat;

extern "C" void initJava() {
	try {
		if (JvCreateJavaVM(NULL) < 0) {
			msg("Error creating the JVM!\n");
			return;		
		}       
		JvAttachCurrentThread(NULL, NULL);
		msg("Java initialized...\n");
	} catch (Throwable *t) {
		msg("Unhandled Java exception!\n");
	}
	return;
}

void ratsel::JIFace::msg(String * jstr) {
	const jchar * chrs =  JvGetStringChars(jstr);
	jsize size = JvGetStringUTFLength(jstr);
	char * str = new char[size+1];
	int i=0;
	for(; i<size; i++) str[i] = chrs[i]; str[i] = 0;
	::msg(str);
	delete[] str;
}

jint ratsel::JIFace::getNetNameN() {
	return ::getNetNameN();
}

String * ratsel::JIFace::getNetName(jint pIdx) {
	return JvNewStringUTF(::getNetName(pIdx));
}

jint ratsel::JIFace::getPinNameN(jint pNetNameIdx) {
	return ::getPinNameN(pNetNameIdx);
}

String * ratsel::JIFace::getPinName(jint pNetNameIdx, jint pPinNameIdx) {
	return JvNewStringUTF(::getPinName(pNetNameIdx, pPinNameIdx));
}

JArray<Pin *> * ratsel::JIFace::getPins() {
	CPin ** pins = ::getPins();
	if (pins == NULL) return NULL;
	int idx = 0;
	while (pins[idx]) ++idx;
	int length = idx;
	JArray<Pin*> *pinA =
       (JArray<Pin*> *)JvNewObjectArray(length, &Pin::class$, NULL);
	for(idx = 0; idx < length; idx++) {
		CPin *pin = pins[idx];
		String *eName = JvNewStringUTF(pin->eName);
		Pin *jpin = new Pin(
			eName, pin->num, pin->seq, pin->x, pin->y, pin->sideMask
		);
		elements(pinA)[idx] = jpin;
	}
	::freePins(pins);
	return pinA;
}

JArray<Rat * > * ratsel::JIFace::getRats() {
	CRat ** rats = ::getRats();
	if (rats == NULL) return NULL;
	int idx = 0;
	while (rats[idx]) ++idx;
	int length = idx;
	JArray<Rat*> *ratA =
       (JArray<Rat*> *)JvNewObjectArray(length, &Rat::class$, NULL);
	for(idx = 0; idx < length; idx++) {
		CRat *rat = rats[idx];
		Rat *jrat = new Rat(
			rat->x0, rat->y0, rat->sideMask0,
			rat->x1, rat->y1, rat->sideMask1,
			rat->id
		);
		elements(ratA)[idx] = jrat;
	}
	::freeRats(rats);
	return ratA;
}

void ratsel::JIFace::selectRat(jint pIdx) {
	::selectRat(pIdx);	
}

extern "C" int ratSelCpp(int argc, const char **argv, int x, int y) {
	msg("C++:hello :)\n");
	jstringArray args = JvConvertArgv(argc, argv);
	try {
		return ratsel::JIFace::ratSel(args, x, y);
	} catch (Throwable *t) {
		msg("Exception caught: "); ratsel::JIFace::msg(t->getMessage()); msg("\n");
	}
	return 0;
}

