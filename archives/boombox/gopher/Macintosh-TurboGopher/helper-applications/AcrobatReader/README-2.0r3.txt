Installation Notes
*******************

Installation Notes for:
=======================
Adobe Acrobat Reader v2.0r3 for Macintosh 
==========================================

Contents: 

 o What's New in Acrobat Reader 2.0r3 for Macintosh 
 o Should I Use Acrobat Reader 2.0r3 or 2.0.1? 
 o Before Installation 
 o System Requirements 
 o Installation Instructions 
 o ATM and Fonts 
 o How much memory to give Acrobat(TM) Reader 2.0r3 
 o Troubleshooting 
 o List of Files Installed 

What's New in Acrobat Reader 2.0r3 for Macintosh
================================================

Acrobat Reader 2.0r3 addresses installation issues related to
different versions of Adobe Type Manager by updating any existing
system-level ATM older than version 3.8.1. It includes changes to
the installer only - there is no change to the application itself.

Should I Use Acrobat Reader 2.0r3 or 2.0.1? 
============================================

There are no feature differences between Acrobat Reader 2.0r3 and
Acrobat Reader 2.0.1. You should choose to use the version
appropriate to your system configuration. If you are
redistributing the Reader, you should consider the system
configurations of your intended audience. 

 o If your system is a 68XXX-based Macintosh without QuickDraw GX,
   use Acrobat Reader 2.0r3. Acrobat Reader 2.0r3 has smaller disk
   and memory requirements, but supports fewer platform
   configurations. 

 o If your system is a Power Macintosh or you have QuickDraw GX
   enabled, use Acrobat Reader 2.0.1. Acrobat Reader 2.0.1
   supports more systems, but has greater disk and memory
   requirements. 

Acrobat Reader 2.0r3 includes ATM LE. To enable Acrobat Reader
2.0r3 and ATM LE to fit on a single floppy, ATM LE does not
include support for QuickDraw GX, Kanji, or 68000-based
Macintoshes, and does not have native Power Macintosh code for
improved performance on Power Macintoshes like the full version of
ATM which is included with Acrobat Reader 2.0.1. The Acrobat
Reader 2.0r3 application is not compatible with QuickDraw GX. 

Acrobat Reader 2.0.1 supports QuickDraw GX and includes full
ATM(TM) 3.8.2. 

Before Installation 
====================

The installation process is controlled by the Acrobat Installer
application. This installation will create a folder named "Adobe
Acrobat 2.0" at the root level on the selected hard disk and
install the new "Acrobat(TM) Reader 2.0r3" application and support
files. 

The Installer will quit any running applications (after prompting
you first). 

System Requirements 
====================

 o Macintosh computer with 68020 (Macintosh II series) or greater
   processor 
 o MacOS 7.0 or greater 
 o 2 MB application RAM 
 o 4 MB hard disk space 
 o FDHD floppy disk drive or CD-ROM player if installing from
   physical media 
 o Acrobat Reader 2.0r3 is not compatible with QuickDraw GX 
 o Acrobat Reader 2.0r3 does not support Kanji or 68000-based
   Macintoshes 

Installation instructions 
==========================

To Install Acrobat(TM) Reader 2.0r3: 

 o Double-click on the "AcroRead.mac" icon and then follow the
   instructions. 
 o See "How Does The Installer Handle Adobe Type Manager (ATM) and
   Fonts?" below for information on ATM and font issues. 
 o See "How much memory to give Acrobat(TM) Reader 2.0r3?" for
   information on optimizing performance. 
 o See the "List of files installed" below for a complete listing
   of the files installed. 

How Does The Installer Handle Adobe Type Manager (ATM) and Fonts?
================================================================

If ATM or SuperATM 3.8 or earlier is installed in the
System/Control Panels folder on your computer, the Acrobat Reader
2.0r3 installer will update it with ATM LE (a special version
3.8.1). It will not update ATM GX (ATM 3.7), ATM 3.5.1J, or ATM
3.8 on Power Macintosh, Kanji, or active QuickDraw GX systems --
see next item. 

Please note that to enable Acrobat Reader 2.0 and ATM LE to fit on
a single floppy, ATM LE does not include support for QuickDraw GX,
Kanji, or 68000-based Macintoshes, and does not have native Power
Macintosh code for improved performance on Power Macintoshes like
the full version of ATM (which is included with Acrobat Reader
2.0.1). You may download Acrobat Reader 2.0.1 without charge from
most online services, or call 800-83-FONTS (800-833-6687) for
information on an upgrade to ATM 3.8.2 or Acrobat Exchange 2.0. 

If you do not have a version of ATM installed in System/Control
Panels folder on your computer, the Acrobat Reader 2.0r3 installer
places a copy of ATM LE in the Adobe Acrobat 2.0 / Fonts folder.
This private copy of ATM is recognized and used only by Acrobat
Reader 2.0r3 and uses memory only when Acrobat Reader is running. 

If TrueType versions of Times, Helvetica, Courier, Symbol, or Zapf
Dingbats are installed on your system, they are not replaced nor
moved. If you are using Type 1 versions of these fonts, updated
versions of the Type 1 fonts Times, Helvetica, Courier, Symbol,
and Zapf Dingbats will be installed in the appropriate location
for your version of System 7. 

Acrobat Reader 2.0 is not compatible with QuickDraw GX. You will
need to disable the ATM GX control panel and the QuickDraw GX
extension. You can use Extensions Manager from the Control Panel
to do this, or remove these files from their locations. See the 
Troubleshooting section belowfor more information on QuickDraw GX
issues. 

How much memory to give Acrobat(TM) Reader 2.0r3?
=================================================

Acrobat(TM) Reader 2.0r3, as it is pre-configured, is set to
require at least 2048K bytes of memory and to prefer 2700K bytes.
To change this, quit the Acrobat(TM) Reader 2.0r3 application if
it is running and select its icon using the Finder. Then select
"Get Info" from the File menu. In the Memory Requirements section
of the dialog, the "factory" default for Minimum size is set to
2048K and should not be changed. The Preferred memory size is
2700K and may be adjusted as described below. 

If you are using ATM LE in the Adobe Acrobat 2.0 / Fonts folder,
ATM LE allocates approximately 576K of RAM from Multi-Finder
memory when it is initialized by Acrobat Reader. If Virtual memory
is on, ATM LE cannot use Multi-Finder memory, so an additional
576K memory must be allocated to Acrobat Reader. 

Like any Macintosh program, increasing the memory size will
benefit performance and increase the number and size of documents
you may have open at once. Setting the "Preferred size" to 4000 (K
bytes) should be sufficient for all but the most complex
documents. 

Troubleshooting 
================

If you experience problems, consider the following: 

 o If you downloaded the Reader from an online service or internet
   site, check that the file transferred properly. Adobe's World
   Wide Web site at http://www.adobe.com/ will list the correct
   size of the binhexed version of the Macintosh 2.0r3 Reader.
   Corrupted archives have been reported to issue a message
   "requires 68020 or greater processor to run" on Macintosh IIx
   systems. 
 o If you have previously installed QuickDraw GX and then
   deinstalled or deactivated it, you may need to reinstall Type 1
   fonts. 
 o The correct process for restoring your original Type 1 font
   configuration may depend on which version of the GX Enabler you
   received. 
    o The QuickDraw GX installer shipped with System 7.5 places
      Type 1 fonts in a folder called * Archived Type 1 fonts *.
      Deinstalling QuickDraw GX does not reinstall these fonts in
      the Fonts folder. 
    o Subsequent versions may treat outline fonts differently. 
 o See the QuickDraw GX Read Me for more details on QuickDraw GX
   operation and compatibility. 
 o Font Not Found - A font not found error may indicate that ATM
   did not have enough memory to create a substitute font.
   Increase the size of the ATM Font Cache from the ATM Control
   Panel. 
 o For best printing results, we recommend you use PSPrinter 8.2.1
   (or later) driver from Adobe. PSPrinter 8.2.1 is not included
   with Acrobat Reader. Contact Adobe at 1-800-521-1976 for
   information on purchasing printer drivers. 
 o If you are using the private copy of ATM in the Adobe Acrobat
   2.0 Fonts folder (i.e. you do not have ATM installed in your
   System's Control Panels folder) background printing is disabled
   in the PSPrinter driver. 
 o If you are printing to a PostScript Level 2 device using the
   Adobe PSPrinter 8.X or Apple LaserWriter 8 printer drivers, you
   need to update the Printer Info after selecting the printer
   from the Chooser in order for the printer driver to recognize
   the printer supports PostScript Level 2. If this is not done,
   Acrobat Reader will not send PostScript Level 2 information to
   the printer. During setup, make sure to select the specific
   PostScript Printer Description (PPD) file for your particular
   printer, do not select the "Generic" PPD if at all possible. 
 o Please note that the Acrobat Reader 2.0r3 does not support
   Kanji or 68000-based Macintoshes. 
 o Current product information for Acrobat is located on Adobe's
   World Wide Web Site at http://www.adobe.com/ 

List of files installed 
========================

The following files are installed by the Acrobat Reader 2.0r3
Installer: 

Adobe Acrobat 2.0 [folder]
     Acrobat(TM) Reader 2.0r3
     ReadMe-Reader
     Fonts [folder]
          ~ATM(TM) (Only if ATM was not previously installed in the System.)
     Help [folder]
          Help-Reader.pdf

System Folder [folder]
           ATM Font Database
     Control Panels [folder]
          ~ATM(TM) (If ATM was previously installed)


