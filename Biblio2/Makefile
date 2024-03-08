CC := gcc
CFLAGS := -Wall -Wextra -pedantic -g
LIBS := libs
SRC := $(wildcard $(LIBS)/*.c)
OBJ := $(SRC:.c=.o)
HEADER := $(wildcard $(LIBS)/*.h)

.PHONY: all clean kill test

all: bibserver bibclient

bibserver: bibserver.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

bibserver.o: bibserver.c
	$(CC) $(CFLAGS) -c $< -o $@

bibclient: bibclient.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread
	
bibclient.o: bibclient.c
	$(CC) $(CFLAGS) -c $< -o $@ 

$(LIBS)/%.o: $(LIBS)/%.c $(HEADER)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f -r $(LIBS)/*.o *.o bibserver bibclient $(OBJ) *.log bib.conf new_bibData 

test : bibserver bibclient
	./bibserver Massa bibData/bib1.txt 3 &
	./bibserver Pisa bibData/bib2.txt 3 &
	./bibserver Siena bibData/bib3.txt 3 &
	./bibserver Firenze bibData/bib4.txt 3 &
	./bibserver Lucca bibData/bib5.txt 3 &
	sleep 1
	./bibclient --autore="ciccio" -p &
	./bibclient --autore="ciccio" --titolo="manuale" -p &
	./bibclient --autore="melis" -p &
	./bibclient --autore="ciccio" --titolo="manuale" &
	./bibclient --autore="ciccio" --titolo="manuale" --anno="2010" -p &
	sleep 1
	./bibclient --autore="kernighan" -p &
	./bibclient --autore="kernighan" --titolo="manuale" -p &
	./bibclient --autore="aho" -p &
	./bibclient --autore="stevens" --titolo="unix" -p &
	./bibclient --autore="kernighan" --titolo="manuale" --anno="2010" &
	sleep 1
	./bibclient --autore="simenon" -p &
	./bibclient --autore="dick" -p &
	./bibclient --autore="aho" -p &
	./bibclient --autore="stevens" --titolo="unix" -p &
	./bibclient --autore="kernighan" --titolo="manuale" --anno="2010" &
	sleep 1
	./bibclient --autore="kernighan" -p &
	./bibclient --autore="kernighan" --titolo="manuale" -p &
	./bibclient --autore="aho" -p &
	./bibclient --autore="stevens" --titolo="unix" -p &
	./bibclient --autore="kernighan" --titolo="manuale" --anno="2010" &
	sleep 1
	./bibclient --titolo="linguaggio" -p &
	./bibclient --luogo_pubblicazione="Reading" &
	./bibclient --nota="Edizione italiana" &
	./bibclient --autore="Taibo" --titolo="Dias" &
	./bibclient --autore="Knuth" -p &
	sleep 1
	./bibclient --titolo="Hijacking" -p &
	./bibclient --editore="Addison" &
	./bibclient --anno="1996" -p &
	./bibclient --autore="luccio" &
	./bibclient --autore="Geronimo Stilton" &
	sleep 1
	./bibclient --nota="divertente" &
	./bibclient --scaffale="A.west.2" -p &
	./bibclient --luogo_pubblicazione="Paris" &
	./bibclient --autore="Marco Briglia" &
	./bibclient --titolo="Mathematical" -p &
	sleep 1
	pkill bibserver
	sleep 5
	chmod +x ./bibaccess.sh
	./bibaccess.sh --query Firenze.log Pisa.log Massa.log Siena.log Lucca.log
	sleep 1
	./bibaccess.sh --loan Firenze.log Pisa.log Massa.log Siena.log Lucca.log

kill:
	pkill bibserver
	
