mkdir -p recv obj
clang -c -o obj/helper.o impl/helper.c -I./
clang -c -o obj/main.o impl/main.c -I./
clang -c -o obj/part.o impl/part.c -I./
clang -c -o obj/receiver.o impl/receiver.c -I./
clang -c -o obj/sender.o impl/sender.c -I./
clang -c -o obj/timer.o impl/timer.c -I./
clang obj/helper.o obj/main.o obj/part.o obj/receiver.o obj/sender.o obj/timer.o -o run 
./run 6543 6543
rm run
