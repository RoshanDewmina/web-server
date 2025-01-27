pipeline {
    agent any

    environment {
        DOCKER_IMAGE = "my_cpp_server"
    }

    stages {
        stage('Checkout') {
            steps {
                // Checkout the source code from the GitHub repository
                git branch: 'main', url: 'https://github.com/...'


                // Gotta complete this

                
            }



        }

        stage('Build Docker Image') {
            steps {
                script {
                    // Build the Docker image
                    sh 'docker build -t $DOCKER_IMAGE .'
                }
            }
        }

        stage('Run Unit Tests') {
            steps {
                script {
                    // Run tests in Docker container
                    sh 'docker run --rm $DOCKER_IMAGE ./run_tests'
                }
            }
        }

        stage('Deploy') {
            steps {
                script {
                    // Deploy the Docker container
                    sh 'docker run -d -p 8080:8080 --name cpp_server_container $DOCKER_IMAGE'
                }
            }
        }

        stage('Cleanup') {
            steps {
                script {
                    // Clean up old containers and images
                    sh 'docker container prune -f'
                    sh 'docker image prune -f'
                }
            }
        }
    }

    post {
        always {
            // Archive test reports and logs (if any)
            archiveArtifacts artifacts: '**/*.log', allowEmptyArchive: true
        }

        success {
            // Send a notification upon successful build
            echo 'Build successful!'
        }

        failure {
            // Send a notification upon failed build
            echo 'Build failed!'
        }
    }
}
