******************************************
* Yet Another Free Raytracer For Windows *
******************************************

TABLE OF CONTENTS
-----------------

0 - Introduction
1 - WARNING!!!
2 - YafRay Options
3 - How to set YafRay in PATH

==================
0 - Introduction
==================

YafRay is a command line raytracer, so if you don't like command line 
programs maybe you're insterested in a GUI. Visit www.yafray.org for 
more info.

The syntax is:

yafray [options] file.xml

==================
1 - WARNING!!!
==================

If you're using YafRay since first versions, please be careful with a possible deprecated yafray.bat in your \windows folder. It could make YafRay fails. You have to delete it.


==================
2 -YafRay Options
==================

-c N 	-> Where 'N' is the number of CPU's to use 
	   (for multi-proccesor computers)
-z	-> Use net optimized (under mosix clusters)
-p PATH -> Where 'PATH' is the installing path of YafRay. By default YafRay
	   uses Windows registry to locate grammar file and plugins dir, so
	   this option is not needed (If all runs fine ;-) ).
-v  	-> Shows YafRay's version.	

NEW in 0.0.6: 

-r min_x:max_x:min_y:max_y 	-> Render region. Values between -1 and 1. 
				   Whole Image is -r -1:1-1:1

For show the options, just type yafray and press ENTER


==============================
HOW TO SET yafray.exe IN PATH
==============================

YafRay Installer doesn't add yafray executable to your PATH environment variable. So if you wish execute YafRay from any directory you need add YafRay's installation directory to the PATH variable yourself.


I will assume that YAFRAY_DIR contains the YafRay installation directory. 
If example, if you have installed YafRay in c:\Program Files\yafray you must
change YAFRAY_DIR with c:\Program Files\yafray in the lines below.

* Setting the path in windows 95/98 
-----------------------------------

- You need to be able to see hidden folders 
- Click on "My Computer" 
- Select "View" from the menu 
- Select "Folder Options" 
- In the window that pops up: 
	- Select the "View" tab 
	- Select "Show all files option" 
	- Select "Ok" 
- Editing AutoExec.bat 
- Open C:\AutoExec.bat with Notepad, Wordpad or other text editor
Two options:

	1.- Append YAFRAY_DIR to the PATH variable.	

	2.- Add the following line to the bottom of your file: 
	SET PATH="YAFRAY_DIR";"%path%"

- Save and close 
- RESTART Windows 


* Setting the path in windows 2000/XP: 
-----------------------------------

- Right click on my computer. 
- Select "Properties". 
- Click "Environment Variables". 

If you have Administrative privileges then you can Edit the Path in 
the System Variables box. If you do not have Adminitrative privileges
then you can Edit the PATH in the user variables box. 

- Select the variable "PATH" or "Path". 
- Click "Edit...". 
- In the box "Variable Value:" go all the way to the end (as far right 
  as possible) you can do this by clicking in the box and then pressing
  and hold in the right arrow on the keyboard. Once you are at the end
  you will need to type the path to the executable program. You will 
  need to type (without the quotes): ";YAFRAY_DIR". 
 
- Click "Ok". 
- Click "Ok". 
- Click "Ok". 

