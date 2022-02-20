CC = g++
CPPFLAGS  = -g -Wall

INCLUDES = -I. -I../ -I../common

default: run 

run: test_simple_fs
	./test_fs

test_simple_fs:  test_simple_fs.o simple_fs.o mock_spi_flash.o
	$(CC) $(CPPFLAGS) $(INCLUDES) -o test_fs test_simple_fs.o simple_fs.o mock_spi_flash.o

mock_spi_flash.o:  mock_spi_flash.cpp simple_fs.h 
	$(CC) $(CPPFLAGS) $(INCLUDES) -c mock_spi_flash.cpp

test_simple_fs.o:  test_simple_fs.cpp simple_fs.h 
	$(CC) $(CPPFLAGS) $(INCLUDES) -c test_simple_fs.cpp

simple_fs.o:  simple_fs.cpp simple_fs.h 
	$(CC) $(CPPFLAGS) $(INCLUDES) -c simple_fs.cpp

clean: 
	$(RM) test_fs *.o *~
