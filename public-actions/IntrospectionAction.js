/**
  *
  * main() will be run when you invoke this action
  *
  * @param Cloud Functions actions accept a single parameter, which must be a JSON object.
  *
  * @return The output of this action, which must be a JSON object.
  *
  */
  
 async function main(params) {
    // TODO Complete this api call
    // this is not completed but hacked to work
    // the client secret would come from your app
    params.client_secret='<client_secret>';
    const introspect_endpoint = '<oAuthServerUrl>/introspect';
    const axios = require('axios');
    const response = await axios.post(introspect_endpoint, params);
    return response.data;
}
