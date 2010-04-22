OUT=test
OBJS=test.o redis.o redisCommand.o redisResponse.o redisSortParams.o redisBuffer.o
CPPFLAGS=-O2 -Wall -Wextra

all: $(OUT)

$(OUT): $(OBJS)
	g++ -o $(OUT) $(OBJS) $(LDFLAGS)

%.o: %.cpp %.h
	g++ -c -o $@ $< $(CPPFLAGS)

%.o: %.cpp
	g++ -c -o $@ $< $(CPPFLAGS)

clean:
	rm -f $(OUT) $(OBJS)

