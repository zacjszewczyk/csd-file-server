default:
	@gcc -Wall -Werror -Wpedantic server.c -o server
	
cwd: default
	@echo "This phony starts the webserver on port 8000, serving from the current working directory."
	@./server -p 8000 -d ./

sl: default
	@echo "This phony starts the webserver on port 8000, serving from the \"~/Documents/splunk-lookups\" directory."
	@./server -p 8000 -d ~/Documents/splunk-lookups/