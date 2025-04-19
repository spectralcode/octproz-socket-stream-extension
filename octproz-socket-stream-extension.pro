QT += core gui widgets network websockets
QMAKE_PROJECT_DEPTH = 0

TARGET = socketstreamextension
TEMPLATE = lib
CONFIG += plugin

#define path of OCTproZ_DevKit share directory, plugin/extension directory
SHAREDIR = $$shell_path($$PWD/../../octproz_share_dev)
PLUGINEXPORTDIR = $$shell_path($$SHAREDIR/plugins)

CONFIG(debug, debug|release) {
	PLUGINEXPORTDIR = $$shell_path($$SHAREDIR/plugins/debug)
}
CONFIG(release, debug|release) {
	PLUGINEXPORTDIR = $$shell_path($$SHAREDIR/plugins/release)
}

#Create PLUGINEXPORTDIR directory if it does not already exist
win32 {
	QMAKE_POST_LINK += $$quote(if not exist "$$PLUGINEXPORTDIR" md "$$PLUGINEXPORTDIR" $$escape_expand(\\n\\t))
} else {
	QMAKE_POST_LINK += $$quote(mkdir -p "$$PLUGINEXPORTDIR" $$escape_expand(\\n\\t))
}


DEFINES += \
	SOCKETSTREAMEXTENSION_LIBRARY \
	QT_DEPRECATED_WARNINGS #emit warnings if depracted Qt features are used

SOURCES += \
	src/broadcaster.cpp \
	src/socketstreamextension.cpp \
	src/socketstreamextensionform.cpp 

HEADERS += \
	src/broadcaster.h \
	src/socketstreamextension.h \
	src/socketstreamextensionform.h  \
	src/socketstreamextensionparameters.h							  

FORMS += \
	src/socketstreamextensionform.ui

INCLUDEPATH += \
	$$SHAREDIR \
	src 



#set system specific output directory for extension
unix{
	OUTFILE = $$shell_path($$OUT_PWD/lib$$TARGET'.'$${QMAKE_EXTENSION_SHLIB})
}
win32{
	CONFIG(debug, debug|release) {
		OUTFILE = $$shell_path($$OUT_PWD/debug/$$TARGET'.'$${QMAKE_EXTENSION_SHLIB})
	}
	CONFIG(release, debug|release) {
		OUTFILE = $$shell_path($$OUT_PWD/release/$$TARGET'.'$${QMAKE_EXTENSION_SHLIB})
	}
}


#specifie OCTproZ_DevKit libraries to be linked to extension project
CONFIG(debug, debug|release) {
	unix{
		LIBS += $$shell_path($$SHAREDIR/debug/libOCTproZ_DevKit.a)
	}
	win32{
		LIBS += $$shell_path($$SHAREDIR/debug/OCTproZ_DevKit.lib)
	}
}
CONFIG(release, debug|release) {
	PLUGINEXPORTDIR = $$shell_path($$SHAREDIR/plugins/release)
	unix{
		LIBS += $$shell_path($$SHAREDIR/release/libOCTproZ_DevKit.a)
	}
	win32{
		LIBS += $$shell_path($$SHAREDIR/release/OCTproZ_DevKit.lib)
	}
}


##Copy extension to "PLUGINEXPORTDIR"
unix{
	QMAKE_POST_LINK += $$QMAKE_COPY $$quote($${OUTFILE}) $$quote($$PLUGINEXPORTDIR) $$escape_expand(\\n\\t)
}
win32{
	QMAKE_POST_LINK += $$QMAKE_COPY $$quote($${OUTFILE}) $$quote($$shell_path($$PLUGINEXPORTDIR/$$TARGET'.'$${QMAKE_EXTENSION_SHLIB})) $$escape_expand(\\n\\t)
}

##Add extension to clean directive. When running "make clean" plugin will be deleted
unix {
	QMAKE_CLEAN += $$shell_path($$PLUGINEXPORTDIR/lib$$TARGET'.'$${QMAKE_EXTENSION_SHLIB})
}
win32 {
	QMAKE_CLEAN += $$shell_path($$PLUGINEXPORTDIR/$$TARGET'.'$${QMAKE_EXTENSION_SHLIB})
}

