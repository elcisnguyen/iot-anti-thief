1. Start cmd
2. "node-red abd"
3. Turn on browser, head to "localhost:1880"
4. The flows is supposed to be there when node-red is on
5. If not, import flows.json
6. Copy bot token and config the telegram botchat node
+ Bot token
+ Chat Id (of your telegram account, by chatting "/start" with userinfobot)
7. Keep your eyes on firebase node, see if it's fully configured or not
+ Firebase path
+ Child Path (Messages)
+ Method (Push/Get)
+ Auth Type (None)
8. Notice MQTT in/out node
+ Topic