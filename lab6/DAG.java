//Anisha Aggarwal	Assignment 2	DAG.java

import java.util.HashSet;
import java.util.Iterator;
import java.util.Scanner;
import java.util.Set;

public class DAG {

	private int distance[];
	private Set<Integer> seen;
	private Set<Integer> unseen;
	private int nodeCount;
	private int adjMatrix[][];

	public DAG(int nodeCount) {

		this.nodeCount = nodeCount;
		this.distance = new int[nodeCount + 1];
		this.seen = new HashSet<Integer>();
		this.unseen = new HashSet<Integer>();
		this.adjMatrix = new int[nodeCount + 1][nodeCount + 1];
	}

	//find the min distance
	int minDist(int dist[], Boolean shortestPath[]) {
		int min = Integer.MAX_VALUE; 
		int min_index = -1;

		for (int i = 0; i < nodeCount; i++) {
			if ((shortestPath[i] == false) && (dist[i] <= min)){
				min = dist[i];
				min_index = i;
			}
		}
		return min_index;
	}

	//find the shortest path in the graph
	int dijkstra(int adj_Matrix[][], int sink, int source) {
		int dist[] = new int[nodeCount];
		Boolean shortestPath[] = new Boolean[nodeCount];

		for (int i = 0; i < nodeCount; i++) {
			dist[i] = Integer.MAX_VALUE;
			shortestPath[i] = false;
		}

		dist[source] = 0;

		for (int j = 0; j < nodeCount - 1; j++) {
			int u = minDist(dist, shortestPath);

			shortestPath[u] = true;

			for (int v = 0; v < nodeCount; v++) {

				if ((!shortestPath[v]) && (adj_Matrix[u][v] != 0) && 
					(dist[u] != Integer.MAX_VALUE) && ((dist[u] + adj_Matrix[u][v]) < dist[v])) {
					dist[v] = dist[u] + adj_Matrix[u][v];
				}
			}
		}
		return dist[sink - 1];
	}

	//find the longest path in the graph
	public int longestPath(int adj_Matrix[][]) {
		int distance_to[] = new int[nodeCount + 1];

		for (int i = 1; i <= nodeCount; i++) {
			distance_to[i] = 0;
		}

		for (int j = 1; j <= nodeCount; j++) {
			for (int k = 1; k <= nodeCount; k++) {
				if (adj_Matrix[j][k] != 0) {
					if (distance_to[k] <= distance_to[j] + 1) {
						distance_to[k] = distance_to[j] + 1;
					}
				}
			}
		}
		return distance_to[nodeCount];
	}

	public int longestPath(int adj_Matrix[][]) {
		int distance_to[];
		distance_to = new int[nodeCount + 1];
		for(int i = 1; i <= nodeCount; i++){
			distance_to[i] = 0;
		}

		for(int j = 1; j <= nodeCount; j++){
			for(int k = 1; k <= nodeCount; k++){
				if(adj_Matrix[j][k] != 0){
					if(distance_to[k] <= distance_to[j] + 1){
						distance_to[k] = distance_to[j] + 1;
					}
				}
			}
		}
		return distance_to[nodeCount];
	}

	public static void main(String[] args) {
		String input;
		Scanner scan = new Scanner(System.in);
		int numGraphs = scan.nextInt();

		for (int i = 0; i < numGraphs; i++){
			int num1;
			int num2;

			int nodeCount = scan.nextInt();
			int edgeCount = scan.nextInt();
			int adj_Matrix[][] = new int[nodeCount + 1][nodeCount + 1];

			for (int j = 0; j < edgeCount; j++) {
				
				input = scan.nextLine();
				
				num1 = scan.nextInt();
				num2 = scan.nextInt();

				adj_Matrix[num1][num2] = 1;
								
			}

			DAG graph = new DAG(nodeCount);
			int shortestPath = graph.dijkstra(adj_Matrix, nodeCount, 1);
			int numPaths = graph.totalPaths(adj_Matrix, nodeCount, 1);

			System.out.println("graph number: " +  (i + 1));
			System.out.println("Shortest path: " + shortestPath);
			System.out.println("Longest path: " + graph.longestPath(adj_Matrix));
			System.out.println("Number of paths: " + numPaths);
			System.out.println("");
		}

		scan.close();
	}
}



