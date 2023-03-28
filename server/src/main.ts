import express from "express";
import cors from "cors";
import dotenv from "dotenv";
import { randomUUID } from "crypto";

const app = express();
export const port = process.env.PORT || 80;
dotenv.config();

app.listen(port, () => {
	console.log(`[server]: Server is running at localhost:${port}`);
});

app.use(cors());
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

const lastEvent: { id: string; type: "commit" | "star" | "alert"; amount: number } = {
	id: "",
	type: "commit",
	amount: 0,
};

app.get("/event", (req, res) => {
	if (req.headers.authorization !== process.env.SECRET) return res.status(401).send("Unauthorized");
	res.send(lastEvent);
});

// recieve webhook from github
app.post("/event", (req, res) => {
	// check for X-GitHub-Event header
	const event = req.headers["x-github-event"];
	const data = req.body;
	console.log("Event: " + event);
	console.log("Data: " + JSON.stringify(data, null, 2));
	switch (event) {
		case "push":
			lastEvent.id = data.after;
			lastEvent.type = "commit";
			lastEvent.amount = data.commits.length;
			break;
		case "star":
			lastEvent.id = randomUUID();
			lastEvent.type = "star";
			lastEvent.amount = 1;
			break;
		case "status":
			if (data.status == "failure" || data.status == "error") {
				lastEvent.id = data.sha;
				lastEvent.type = "alert";
				lastEvent.amount = 1;
			}
			break;
		default:
			break;
	}

	res.json({ success: true });
});
