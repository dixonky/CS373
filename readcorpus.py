#!/usr/bin/python
#Kyle Dixon
import json
import sys
import getopt
import os

#Main
def main(argv):
    file=''
    myopts, args = getopt.getopt(sys.argv[1:], "", ["file="])    #https://pymotw.com/2/getopt/
    for o, a in myopts:
        if o in ('-f, --file'):
            file=a

    corpus = open(file)
    urldata = json.load(corpus, encoding="latin1")
    if urldata and urldata[0]['malicious_url'] is not None:
        train(urldata)        #If the URL list is already classified 
    else:
        classify(urldata)      #Using the program to classify as list
    corpus.close()


#Malicous test function
def maliciousTest(record):
    score = malwareScore(record)     #Run the malwareScore function
    return score < 0                #Return the score if it passes the malware threshold (negative value)


#Whitelists and Blacklists
TLD_WL = {"com", "org", "net", "edu"}       #These lend credibility
HOST_WL = (".yimg.com", ".microsoft.com",".cloudfront.net", ".yahoo.co.jp")     #Common good guys
TLD_BL = {"br", "ru", "vu"}     #troublesome areas
WORD_BL = ["paypal", "googledocs", "googledrive"]       #These destroy credibility


#Malware Score Function
        #A big checklist of evidence
def malwareScore(record):
    score = 0                  #Start score at the standard/baseline
    if record['registered_domain'] is None:     #Setup the registered domain
        record['registered_domain'] = ""
    if not record['ips']:           #Check for DNS
        score -= 2
    if int(record["domain_age_days"]) < 50:     #Check Domain Age
        score -= 1
    elif int(record["domain_age_days"]) > 365:
        score += 1
    subdomain = fixsuff(record['host'], record['registered_domain'])    #Check top level domain names
    subdomain_tokens = subdomain.split('.')
    if any(tld in subdomain_tokens for tld in ("com", "net", "org")):   #Trying to hide something
        score -= 5
    if "www" in record['domain_tokens'][:1]:        #Check for www
        score += 1
    if '/www.' in record['path'] or '.com/' in record['path']:
        score -= 1
    numH = sum(c.isdigit() for c in record['host'])  #Check for numbers in the host name
    if numH >= 3:
        score -= 1
    if 'wp-admin' in record['path_tokens'] or 'wp-includes' in record['path_tokens']:       #Check for wordpress
        score -= 2
    if record['host'].endswith(HOST_WL):     #Check Whitelists and Blacklists
        score += 1
    if record['tld'] in TLD_WL:             
        score += 1
    if record['tld'] in TLD_BL:
        score -= 1
    if any(word in record['url'] for word in WORD_BL):      
        score -= 2
    if record.get('alexa_rank') is not None:        #Check Alexa rank
        alexa_rank = int(record['alexa_rank'])
        if alexa_rank > 20000:                      #malware appears to be above this rank
            score -= 2
        elif alexa_rank < 10000:                    #I wish this was still free :(
            score += 2
    else:                                           #Not having a score is not good (Future modifications could at a time check to this aka new sites wouldnt be hurt)
        score -= 2
    if record['file_extension'] == 'php':       #Check php
        score -= 2
    if record['file_extension'] == 'htm':       #Check htm
        score -= 2
    if record['port'] != (80):     #Check standard port
        score -= 1
    if record['num_path_tokens'] > 10:      #Check path length
        score -= 2
    elif record['path_len'] > 70:
        score -= 1
    elif len(record['url']) > 70:
        score -= 1

    return score


def fixsuff(s, suffix):
    if s.endswith(suffix):
        return s[:-len(suffix)]
    return s


#train Function
def train(urldata):
    matrix = [[0, 0], [0, 0]]
    for record in urldata:
        prediction = bool(maliciousTest(record))
        actual = bool(record['malicious_url'])
        matrix[actual][prediction] += 1
    print("positives: ", matrix[1][1])
    print("negatives: ", matrix[0][0])
    print("false negatives (Type 2 E): ", matrix[1][0])
    print("false positives (Type 1 E): ", matrix[0][1])


#classify function
def classify(urldata):
    positives  = 0
    negatives = 0
    for record in urldata:
        prediction = bool(maliciousTest(record))
        if prediction == True:
            positives += 1
        else:
            negatives += 1
        print("{}, {}".format(record['url'], int(prediction)))
    print("positives:", positives)
    print("negatives:", negatives)


if __name__ == "__main__":
     main(sys.argv[1:])

