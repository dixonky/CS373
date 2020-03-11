#!/usr/bin/python

#Kyle Dixon
#Sources: https://docs.python.org/3/howto/sorting.html
    #https://docs.python.org/3/howto/argparse.html   https://stackoverflow.com/questions/12818146/python-argparse-ignore-unrecognised-arguments
from CSVPacket import Packet, CSVPackets
import sys
import argparse


#Stats Function
    #Sum the packets sent to each protocol and port
def statsProtocol():
    print('Getting protocol stats...')
    TCP = [0 for x in range(1025)]      #Setup holders (we are allowing for ports 1 to 1024)
    UDP = [0 for x in range(1025)]
    for pkt in CSVPackets(csvfile):     #Loop through the packets
        protocol = pkt.proto & 0xff     #Get the protocol (6 = TCP, 17 = UDP)
        if protocol == 6 and pkt.tcpdport <= 1024:          #Add to port count if appropriate
            TCP[pkt.tcpdport] += 1
        elif protocol == 17 and pkt.udpdport <= 1024:       #Add to port count if appropriate
            UDP[pkt.udpdport] += 1
    for index, item in enumerate(TCP):      #Print the saved port counts
        if item > 0:
            print('TCP Port: ', index, '  packets: ', item)
    for index, item in enumerate(UDP):
        if item > 0:
            print('UDP Port: ', index, '  packets: ', item)


#Starter Function
    #Starter code function
def starter():
    IPProtos = [0 for x in range(256)]
    numBytes = 0
    numPackets = 0
    for pkt in CSVPackets(csvfile):
        numBytes += pkt.length
        numPackets += 1
        proto = pkt.proto & 0xff
        IPProtos[proto] += 1
    print "numPackets:%u numBytes:%u" % (numPackets,numBytes)
    for i in range(256):
        if IPProtos[i] != 0:
            print "%3u: %9u" % (i, IPProtos[i])


#Count IP Function
    #Sums the packets sent to each IP address and prints ordered list
    #Checks for additional parameter to specificy packets counted
def countIP():
    IP = []     #use for now, will transfer to a dict later for sorting
    if len (sys.argv) == 4:     #Check for a specific condition via the fourth argument
        if (sys.argv[3] == "-GRE"):     #GRE comes through 47
            for pkt in CSVPackets(csvfile):
                if pkt.proto == 47:
                    IP.append(pkt.ipsrc)
                    IP.append(pkt.ipdst)
        elif (sys.argv[3] == "-IPSEC"):
            for pkt in CSVPackets(csvfile):
                if pkt.proto == 50 or pkt.proto == 51:
                    IP.append(pkt.ipsrc)
                    IP.append(pkt.ipdst)
        elif (sys.argv[3] == "-OSPF"):
            for pkt in CSVPackets(csvfile):
                if pkt.proto == 89:
                    IP.append(pkt.ipsrc)
                    IP.append(pkt.ipdst)
    else:                       #No special condition, get all
        for pkt in CSVPackets(csvfile):
            IP.append(pkt.ipsrc)
            IP.append(pkt.ipdst)

    Dict = { x : IP.count(x) for x in set(IP)}      #create a dictionary to sort the list
    sortDict = sorted(Dict.items(), key=lambda kv: kv[1], reverse=True)     #sort the dictionary in a new sorted dictionary https://docs.python.org/3/howto/sorting.html
    for x in sortDict:      #Print appropriate response
        print x   
    if not IP:
        print ("None Found")


#Connection Function
    #Prints the unique ipsrc on each protocol and port
def conn():
    IPDict = {}     #We are going to want to take advantage of sorting
    for pkt in CSVPackets(csvfile):
        flag = 0
        if pkt.proto == 6:      #TCP
            if pkt.tcpdport <= 1024:
                pp = "tcp/" + str(pkt.tcpdport)
                flag = 1
        elif pkt.proto == 17:   #UDP
            if pkt.tcpdport <= 1024:
                pp = "udp/" + str(pkt.udpdport)
                flag = 1
        if flag == 1:           #Add values if data port number was appropriate
            if pkt.ipdst in IPDict:
                IPDict[pkt.ipdst][0].add(pkt.ipsrc)
                IPDict[pkt.ipdst][1].add(pp)
            else:
                IPDict[pkt.ipdst] = [set([pkt.ipsrc]), set([pp])]
    i=0
    for k,v in sorted(IPDict.items(), key=lambda (k,v): (len(v[0]), k), reverse=True): #https://docs.python.org/3/howto/sorting.html
        i+=1
        print ("ipdst ", k, " has ", len(v[0]), " distinct ipsrc on ports: ", v[1])
        if i > 20:      #Lab directions call for top 20 entries
            break 


#START
    #Calls a function depending on the passed in arguments
csvfile = open(sys.argv[1],'r')
    #I used argparse here as a learning exercise for myself
parser = argparse.ArgumentParser()   #https://docs.python.org/3/howto/argparse.html
parser.add_argument("-stats", help="Get the protocol stats", action="store_true")
parser.add_argument("-start", help="Run starter script", action="store_true")
parser.add_argument("-countip", help="Run starter script", action="store_true")
parser.add_argument("-connto", help="Run starter script", action="store_true")
args, unknown = parser.parse_known_args() #https://stackoverflow.com/questions/12818146/python-argparse-ignore-unrecognised-arguments
if args.stats:
    statsProtocol()
if args.start:
    starter()
if args.countip:
    countIP()
if args.connto:
    conn()

