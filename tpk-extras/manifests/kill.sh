echo "Scrubbing the LKE cluster clean for a new STK deployment."
kubectl --kubeconfig=kc-develop delete deployments stk-tpk-server
kubectl --kubeconfig=kc-develop delete services supertuxkart-service
