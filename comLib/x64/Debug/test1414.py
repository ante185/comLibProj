#!/bin/python
__author__ = 'fll'

import threading
import subprocess
import time

import sys

programPath = 'D:/repos/comLibProj/comLib/x64/Debug/'
programName = 'comLib.exe'

tests = [
	{
		 # One server with 1 ms delay,
		 # one client with 8 ms delay,
		 # The Server will have to wait for the client
		 # to advance, stressing bufferFull condition
		 'testname': 'client delay',
		 'serverDelay': 1,
		 'clientsDelays': [8],
		 'numMsg': 5000,
		 'memSize': 64,
		 'msgSize': 1024
	},
	{
		 # One server with 8 ms delay,
		 # One client with 1 ms delay,
		 # The Client will have to wait for the server
		 # to advance, stressing bufferEmpty condition
		 'testname': 'server delay',
		 'serverDelay': 8,
		 'clientsDelays': [1],
		 'numMsg': 5000,
		 'memSize': 64,
		 'msgSize': 1024
	},
	{
		# One server with 1 ms delay,
		# One client with 1 ms delay,
		# Random size
		'testname': 'random message size',
		'serverDelay': 1,
		'clientsDelays': [1],
		'numMsg': 20000,
		'memSize': 64,
		'msgSize': "random"
	}
]

# one thread per command
class ThreadTest(threading.Thread):
	def __init__(self, cmd):
		self.stdout = None
		self.stderr = None
		self.cmd = cmd
		threading.Thread.__init__(self)

	def run(self):
		p = subprocess.Popen( self.cmd.split(),
						 shell = False,
						 stdout = subprocess.PIPE,
						 stderr = subprocess.PIPE )
		self.stdout, self.stderr = p.communicate()

if __name__=='__main__':

	for test in tests:
		# repeat each test N times!
		for run in range(2):
			server = None
			clients = []
			# this string repeats for all executions
			print("=====================================")
			print("Running test: " + test['testname'])
			msgConf = "%s %s %s" % (test['memSize'],test['numMsg'],test['msgSize'])
			time.sleep(0.1)
			startTime = time.perf_counter()

			# initiate clients:
			for clientDelay in test['clientsDelays']:
				commandLine = "%s consumer %s %s" % (programPath + programName, clientDelay, msgConf)
				clients.append ( ThreadTest (commandLine) )
				clients[-1].start()
			
			time.sleep(0.1)
			commandLine = "%s producer %s %s" % (programPath + programName, test['serverDelay'], msgConf)
			server = ThreadTest(commandLine)
			server.start()

			server.join()
			for c in clients:
				c.join()

			endTime = time.perf_counter()
			elapsed = endTime - startTime


			print("Time elapsed: ", elapsed)

			result = server.stdout

			for index, c in enumerate(clients):
				if (result != c.stdout):
					f = open(programPath + "Test_" + test['testname'] + "__Run_" + str(run) + "__Client_" + str(index) + "__log.txt", "w")
					f.write("The log contains the mismatching messages printed by the server and client.\nThe messages in this log consists of first part of each message, up to a size of memSize in kilobytes * 512. (Which should be half your total memory, and the max size of a message)\n\n")
			
					serverStrBase = str(result, 'utf-8')
					clientStrBase = str(c.stdout, 'utf-8')#[1:len(str(c.stdout))]
					
					serverStrSplit = serverStrBase.splitlines(True)
					clientStrSplit = clientStrBase.splitlines(True)
					
					f.write("Server total messages sent: %d (%d bytes)\nClient total messages read: %d (%d bytes)\n\n" % (len(serverStrSplit), len(serverStrBase), len(clientStrSplit), len(clientStrBase)))
					
					for serverStr, clientStr in zip(serverStrSplit, clientStrSplit):
						if serverStr != clientStr:
							if len(serverStr) > test['memSize'] * 512:
								f.write("Server (Msg byte size %d): %s\n" % (len(serverStr), repr(serverStr[:test['memSize'] * 512])))
							else:
								f.write("Server (Msg byte size %d): %s\n" % (len(serverStr), repr(serverStr)))

							if len(clientStr) > test['memSize'] * 512:
								f.write("Client (Msg byte size %d): %s\n\n" % (len(clientStr), repr(clientStr[:test['memSize'] * 512])))
							else:
								f.write("Client (Msg byte size %d): %s\n\n" % (len(clientStr), repr(clientStr)))
					
					f.close();
					
					print("Test failed. See " + programPath + "Test_" + test['testname'] + "__Run_" + str(run) + "__Client_" + str(index) + "_log.txt" + " for more information.")
					
				else:
					print("Test passed")
			print("=====================================")




