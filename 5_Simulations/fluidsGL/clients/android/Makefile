ADT := ~/Downloads/adt-bundle-linux-x86_64-20140702
ANT := ant
ADB := $(ADT)/sdk/platform-tools/adb
ANDROID := $(ADT)/sdk/tools/android
NDK := ~/Downloads/android-ndk-r10b
NDK_BUILD := $(NDK)/ndk-build

.PHONY: test install build configure

all: test

test: install
	$(ADB) shell am start -n com.example.caffemacchiato/.FullscreenActivity
# && adb logcat

install: build
	$(ADB) install -r ./bin/FullscreenActivity-debug.apk

build:
	cd jni && $(NDK_BUILD) && cd .. && $(ANT) debug

configure:
	$(ANDROID) update project --path . --subprojects --target "android-19"

clean:
	$(ANT) clean
