CC	= gcc
CFLAGS	= -Wall -fopenmp
LDFLAGS	=
LIBS	= -lm
OBJS	= motionEstimation.o bitmapFileHeader.o picture.o setCTU.o Command.o profile.o
PROGRAM	= motionEstimation

all:		$(PROGRAM)

$(PROGRAM):	$(OBJS)
		$(CC) $(LDFLAGS) -fopenmp -o $@ $(OBJS) $(LIBS)

clean:;		rm -f *.o *~
cls:;		rm -f *.o *~ *.obj *.tds $(PROGRAM)
###
motionEstimation.o:	motionEstimation.h motionEstimation.c
bitmapFileHeader.o:	motionEstimation.h bitmapFileHeader.h bitmapFileHeader.c
picture.o         :	motionEstimation.h picture.c
setCTU.o          :	motionEstimation.h setCTU.c
Command.o         :	motionEstimation.h Command.c
profile.o    	  :	motionEstimation.h profile.c