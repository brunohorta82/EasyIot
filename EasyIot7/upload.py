import requests


myobj = {'file':{}}

with open('build/PRO100.bin', 'rb') as f:
    data = f.read()
r = requests.post("https://update.bhonofre.pt/firmware/upload", data=data,headers = {"Api-Key": "6e48937e-71bc-11ee-b962-0242ac120002"})

print(r.status_code)