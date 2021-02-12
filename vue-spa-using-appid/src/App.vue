<template>
  <div id="app">
    <h1>IBM App ID Demo using Vue and ibmcloud-appiD-js</h1>
    <p>{{ msg }}</p>
    <button v-on:click='loginWithAppID'>Click me to login with IBM App ID</button>
    <p>{{ private_message }}</p>
  </div>
</template>

<script>
import AppID from "ibmcloud-appid-js"
import 'regenerator-runtime/runtime'
import { IBM_APP_ID_CLIENTID, IBM_APP_ID_DISCOVERY_ENDPOINT } from "./env.js"

export default {
  name: 'app',
  data () {
    return {
      msg: 'Welcome to Your Vue.js App',
      private_message: ''
    }
  },
  async created() {    
    
  },
  methods: {
    async loginWithAppID() {
      // launches login screen on start
      // assumes it is returning from the domain its launched from
      const appID = new AppID();
      await appID.init({
        clientId: IBM_APP_ID_CLIENTID,
        discoveryEndpoint: IBM_APP_ID_DISCOVERY_ENDPOINT
      });
      const tokens = await appID.signin();
      const userInfo = await appID.getUserInfo(tokens.accessToken)
      console.log(userInfo)
      fetch("https://31f5ff35.eu-gb.apigw.appdomain.cloud/private-authtest/Hello", {
      headers: {
        "Content-type": "application/json",
        Authorization: `Bearer ${tokens.accessToken}`
      }
      }).then((res) => res.json())
      .then(json => { console.log(json); return json;} )
      .then(json => this.private_message = json.message+", user is "+userInfo.name)
      .catch((err) => {
        console.log(err)
      });

      // this does not support the refresh token so if the user refreshes then they will
      // need to login.  You could cache the asset token somewhere but this is not recommended.
    }
  }
}
</script>

<style>
#app {
  font-family: 'Avenir', Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  color: #2c3e50;
  margin-top: 60px;
}

h1, h2 {
  font-weight: normal;
}

ul {
  list-style-type: none;
  padding: 0;
}

li {
  display: inline-block;
  margin: 0 10px;
}

a {
  color: #42b983;
}
</style>
