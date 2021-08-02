from web3 import Web3
# print(Web3.toWei(1, 'ether'))
# print(Web3.fromWei(500000000, 'gwei'))

w3 = Web3(Web3.EthereumTesterProvider())
# print(w3.isConnected())

account = w3.eth.accounts[0]
wei = w3.eth.get_balance(account)
w3.fromWei(wei, 'ether')

tx_hash = w3.eth.send_transaction({
    'from': w3.eth.accounts[0],
    'to': w3.eth.accounts[1],
    'value': w3.toWei(3, 'ether')
})

# print('=======transaction=========')
# print(w3.eth.get_transaction(tx_hash))
# print('=======end=========')

# print('=======block===============')
# print(w3.eth.get_block('latest'))
# print('=======end===============')

print(w3.fromWei(w3.eth.get_balance(w3.eth.accounts[0]), 'ether'))

print(w3.fromWei(w3.eth.get_balance(w3.eth.accounts[1]), 'ether'))
