# ZEROTRACE - disk wiping and verification solution

The utility will have 2 main parts:
    1. zt-client
    2. zt-verify

* zt-client will be the main drive wiping utility which will read device properties, carry out wipe and also present a nice GUI on top.

* zt-verify will be the eth-based smart contract verification system. Everytime a device is wiped a certficate will be generated and sstored on IPFS. The IPFS hash will be stored along with device uuid on the block chain. This way the wipe status can be verified.
For more info see [[https://medium.com/@sahilkadam257/certificate-validation-using-blockchain-3c560fd1738c]]

