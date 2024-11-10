# DEPLOY THE SUPERTUXKART CLUSTER & APP

_Chris Finch / cfinch@akamai.com / 2024-10-14_

The following instructions walk you through the process of getting a STK server running in an LKE cluster.  

## Create the binary

Make changes as needed to the codebase in `./stk-code` and make the binary

1. Navigate to `./stk-code/cmake_build` and run the `makeme.sh` file. This will take two steps:
    1. Run `cmake` to build the server as headless only, and
    2. Run `make` to create the binary, which will be found in `./stk-code/cmake_build/bin`



## Confirm the Server Runs

At this point, assuming the binary was built, the server should run correctly. You can run a few tests to quickly determine if all was successful.

1. Navigate to `./stk-source/cmake_build/bin` and execute the `supertuxkart` binary as a headless server with the following command-line entry:
    `./supertuxkart --server-config=stk-tpk-config.xml --lan-server=stk-tpk-server --network-console`
2. Verify that the server has started - the last message available after server start should read:
    `<date><time><year> [info   ] STKHost: Listening has been started.`
3. Download the SuperTuxKart client to your computer - available [here](https://supertuxkart.net/Download).
4. Run the client and connect to `localhost` on port `2759`. You should be able to connect to a lobby and start a game.



## Configure the server

Now that you've made your changes to the game and confirmed it's correctly built, we need to add critical information to the game's configuration file in order to have it point to the TrafficPeak analytics endpoint. Let's do that now.

1. Navigate to `./stk-source/cmake_build/bin` and open up the `stk-tpk-config.xml` file.
2. Alter the following fields, which can be found at the end of the file:

    <!-- Token for TPK endpoint, required for analytics to function. -->
    <tpk-token value="<your-trafficpeak-token-here>" />
    
    <!-- Name of the TPK Table analytics will be dropped into; required for analytics to function. -->
    <tpk-table value="<your-trafficpeak-table-id-here>" />
    
    <!-- HTTPS endpoint for analytics data; required for analytics to function. -->
    <tpk-url value="https://your.trafficpeak.uri/" />
    
    <!-- Basic Auth UID for TPK endpoint, required for analytics to function. -->
    <tpk-uid value="<your-trafficpeak-basic-auth-uid>" />
    
    <!-- Basic Auth PWD for TPK endpoint, required for analytics to function. -->
    <tpk-pwd value="<your-trafficpeak-basic-auth-pwd>" />



## Build the Docker image

You'll need to containerize the binary to get it working on LKE. The Dockerfile as provided creates the environment required to support the application from scratch, assuming both the STK codebase and asset library have been downloaded per the instructions found at the [SuperTuxKart Github Repository](https://github.com/supertuxkart/stk-code/tree/master). Since this has been done, you can take the following steps:

1. Navigate to the root of the project.
2. Execute the `docker build -t stk-tpk-server .` to complete
3. Ensure you've logged into Docker Hub using `docker login`
4. Tag the image as latest by executing `docker tag stk-tpk-server <your-dockerhub-id>/stk-tpk-server:latest`
5. Push the image to Docker Hub by executing `docker push <your-dockerhub-id>/stk-tpk-server:latest`



## Stand up the LKE Cluster

We'll need to stand up the hardware to support the game server as well as grabbing the kubeconfig file that will allow us to load the container on to the server cluster. Fortunately, with Terraform, this is a simple process at the command line, and we've provided scripts which will do this automatically.

1. Navigate to `./terraform` and execute `terraform plan -var-file=dev.tfvars` to validate the TF files.
2. Execute `terraform apply -var-file=dev.tfvars` to instantiate the cluster we need.
3. Note that the output of the process will be a file in the `./terraform` directory called `kc-default`: this is the `kubeconfig` file for the LKE cluster you've just created.



## Apply the manifests and run the Server

The manifests that have been provided in the `./manifests` directory provide a basic service deployment which will result in a playable server. These are _not_ recommended for production (for instance, you'll probably want to implement MetalLB instead of using NodePort for exposing game ports) but will function perfectly well for a demonstration.

You'll need to apply the Kubernetes Manifests to get the game running in the cluster. Follow this process:

- Copy the `kc-default` file to the `./manifests` directory.
- Use kubectl to apply the manifests by executing `kubectl --kubeconfig=kc-default apply -f deployment.yaml -f service.yaml`
- Verify the resources are created by issuing `kubectl --kubeconfig=kc-default get deployments` 
- Verify the deployment by issuing `kubectl --kubeconfig=kc-default get pods` and retrieve the name of one of your working pods.



## Play the game!

With this completed, the game server should be upright and ready to receive connections from game clients!

- Validate that the STK server is running by viewing the STK startup log sequence: issue `kubectl --kubeconfig=kc-default logs <podname>`

- Assuming that all is well, obtain the Node IPs exposed with `kubectl --kubeconfig=kc-default get nodes -o wide`
- Also, obtain the Port numbers exposed with `kubectl --kubeconfig=kc-default get services -o wide` - the UDP port that maps to 2759 will be the port we want to connect to with the SuperTuxKart client.

- Load up the SuperTuxKart client and connect to the server using the IP and Port information obtained in the prior step.
    - After loading the application, click 'Online'
    - Then click "Enter Server Address"
    - If you have not previously entered the IP or port, enter the IP and Port into the text box using `XXX.XXX.XXX.XXX:PPPPP` notation.
    - Click OK

You should now be connected to the Game Server lobby. Go ahead and click 'Start Game,' then select a racer and a kart, and get playing!